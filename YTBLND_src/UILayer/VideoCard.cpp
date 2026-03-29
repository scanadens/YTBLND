// ============================================================================
// VideoCard.cpp — Custom video preview card widget implementation
// ============================================================================

#include "VideoCard.hpp"

#include <thread>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/url.h>
#include <wx/sckstrm.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/utils.h>   // wxLaunchDefaultBrowser

#include "UIColors.hpp"

// ---------------------------------------------------------------------------
// Event definition
// ---------------------------------------------------------------------------
wxDEFINE_EVENT(EVT_THUMBNAIL_LOADED, ThumbnailEvent);

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------
VideoCard::VideoCard(wxWindow* parent, AppController& controller)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(280, 220))
    , m_controller(controller)
    , m_thumbLoading(false)
    , m_thumbFailed(false)
    , m_generation(0)
    , m_hovered(false)
{
    SetBackgroundColour(UIColors::Surface);
    SetBackgroundStyle(wxBG_STYLE_PAINT);   // suppress default erase so OnPaint owns it

    Bind(wxEVT_PAINT,       &VideoCard::OnPaint,       this);
    Bind(wxEVT_ENTER_WINDOW,&VideoCard::OnMouseEnter,  this);
    Bind(wxEVT_LEAVE_WINDOW,&VideoCard::OnMouseLeave,  this);
    Bind(wxEVT_LEFT_UP,     &VideoCard::OnClick,       this);
    Bind(EVT_THUMBNAIL_LOADED, &VideoCard::OnThumbnailLoaded, this);

    // Make sure wxURL / wxImage JPEG handler is available
    if (!wxImage::FindHandler(wxBITMAP_TYPE_JPEG)) {
        wxImage::AddHandler(new wxJPEGHandler());
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void VideoCard::SetVideo(const Video& video)
{
    m_videoID    = wxString::FromUTF8(video.getVideoID());
    m_title      = wxString::FromUTF8(video.getTitle());
    m_channelID  = wxString::FromUTF8(video.getChannelID());

    // Reset thumbnail state for the new video
    m_thumbnail   = wxBitmap();
    m_thumbLoading = false;
    m_thumbFailed  = false;

    FetchThumbnailAsync();
    Refresh();
}

void VideoCard::Clear()
{
    m_videoID   = wxEmptyString;
    m_title     = wxEmptyString;
    m_channelID = wxEmptyString;

    m_thumbnail    = wxBitmap();
    m_thumbLoading = false;
    m_thumbFailed  = false;

    Refresh();
}

// ---------------------------------------------------------------------------
// Async thumbnail fetch
// ---------------------------------------------------------------------------
void VideoCard::FetchThumbnailAsync()
{
    if (m_videoID.IsEmpty()) return;

    m_thumbLoading = true;
    m_thumbFailed  = false;
    ++m_generation; // invalidate any in-flight fetch for the previous video

    // Capture everything by value — the thread must be self-contained
    wxString   videoID = m_videoID;
    VideoCard* card    = this;        // wxQueueEvent is safe if card is gone
    int        gen     = m_generation;

    std::thread([videoID, card, gen]() {
        wxString url = wxString::Format(
            "http://img.youtube.com/vi/%s/hqdefault.jpg", videoID);

        bool    ok  = false;
        wxImage img;

        wxURL wxurl(url);
        if (wxurl.GetError() == wxURL_NOERR) {
            wxInputStream* stream = wxurl.GetInputStream();
            if (stream && stream->IsOk()) {
                wxMemoryOutputStream memOut;
                stream->Read(memOut);
                delete stream;

                wxMemoryInputStream memIn(memOut);
                if (img.LoadFile(memIn, wxBITMAP_TYPE_JPEG) && img.IsOk()) {
                    ok = true;
                }
            } else {
                delete stream;
            }
        }

        // Post result back to the UI thread; silently dropped if card is gone
        auto* evt = new ThumbnailEvent(EVT_THUMBNAIL_LOADED, wxID_ANY, img, ok, gen);
        wxQueueEvent(card, evt);
    }).detach();
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------
void VideoCard::OnThumbnailLoaded(ThumbnailEvent& evt)
{
    // Discard results from a fetch that was superseded by a later SetVideo() call
    if (evt.generation != m_generation) return;

    m_thumbLoading = false;
    if (evt.success && evt.image.IsOk()) {
        m_thumbnail   = wxBitmap(evt.image);
        m_thumbFailed = false;
    } else {
        m_thumbFailed = true;
    }
    Refresh();
}

void VideoCard::OnMouseEnter(wxMouseEvent& /*evt*/)
{
    m_hovered = true;
    Refresh();
}

void VideoCard::OnMouseLeave(wxMouseEvent& /*evt*/)
{
    m_hovered = false;
    Refresh();
}

void VideoCard::OnClick(wxMouseEvent& /*evt*/)
{
    if (m_videoID.IsEmpty()) return;

    // Open in browser
    wxString browserURL = wxString::Format(
        "https://www.youtube.com/watch?v=%s", m_videoID);
    wxLaunchDefaultBrowser(browserURL);

    // Dispatch event to controller
    EventPayload payload;
    payload["videoID"] = std::string(m_videoID.ToUTF8());
    m_controller.getEventRouter().dispatch("playVideo", payload);
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

// Helper: measure text line height using the graphics context font
static double LineHeight(wxGraphicsContext* gc)
{
    double tw, th, td, tex;
    gc->GetTextExtent("Wg", &tw, &th, &td, &tex);
    return th;
}

// Draw text wrapped to maxLines lines, ellipsising on the last line.
// Returns the y-coordinate directly below the last rendered line.
int VideoCard::DrawWrappedText(wxGraphicsContext* gc,
                               const wxString&    text,
                               int                maxLines,
                               double x, double y,
                               double maxWidth) const
{
    if (text.IsEmpty()) return static_cast<int>(y);

    double tw, th, td, tex;
    gc->GetTextExtent("Wg", &tw, &th, &td, &tex);
    double lineH = th + 2.0;

    // Split into words
    wxArrayString words = wxSplit(text, ' ');
    wxString currentLine;
    int      lineCount = 0;

    auto flush = [&](const wxString& line, bool isLast) {
        if (lineCount >= maxLines) return;
        wxString toDraw = line;
        gc->GetTextExtent(toDraw, &tw, &th, &td, &tex);
        if (!isLast || tw > maxWidth) {
            // Ellipsis trimming
            while (!toDraw.IsEmpty()) {
                wxString candidate = toDraw + L"\u2026";
                gc->GetTextExtent(candidate, &tw, &th, &td, &tex);
                if (tw <= maxWidth) { toDraw = candidate; break; }
                toDraw.RemoveLast();
            }
        }
        gc->DrawText(toDraw, x, y + lineCount * lineH);
        ++lineCount;
    };

    for (size_t i = 0; i < words.GetCount(); ++i) {
        wxString candidate = currentLine.IsEmpty()
                             ? words[i]
                             : currentLine + " " + words[i];
        gc->GetTextExtent(candidate, &tw, &th, &td, &tex);
        if (tw > maxWidth && !currentLine.IsEmpty()) {
            flush(currentLine, false);
            if (lineCount >= maxLines) goto done;
            currentLine = words[i];
        } else {
            currentLine = candidate;
        }
    }
    if (!currentLine.IsEmpty()) {
        flush(currentLine, true);
    }
done:
    return static_cast<int>(y + lineCount * lineH);
}

void VideoCard::OnPaint(wxPaintEvent& /*evt*/)
{
    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) return;

    const wxSize  sz      = GetClientSize();
    const double  W       = sz.GetWidth();
    const double  H       = sz.GetHeight();

    // ── Card background ──────────────────────────────────────────────────
    gc->SetBrush(wxBrush(UIColors::Surface));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(0, 0, W, H, 10.0);

    // ── Thumbnail area (top 72%) ─────────────────────────────────────────
    const double thumbH = H * 0.72;

    if (m_thumbnail.IsOk()) {
        // Scale to fill the thumbnail area
        wxImage scaled = m_thumbnail.ConvertToImage()
                                    .Scale(static_cast<int>(W),
                                           static_cast<int>(thumbH),
                                           wxIMAGE_QUALITY_BILINEAR);
        wxBitmap bmp(scaled);
        gc->DrawBitmap(bmp, 0, 0, W, thumbH);
    } else {
        // Placeholder rectangle
        gc->SetBrush(wxBrush(UIColors::SurfaceRaised));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(0, 0, W, thumbH);

        // Centered label
        wxString label;
        if (m_thumbLoading) {
            label = "Loading...";
        } else if (m_thumbFailed) {
            label = L"\u25B6";   // ▶ play icon
        } else {
            // No video set — blank placeholder
            label = wxEmptyString;
        }

        if (!label.IsEmpty()) {
            wxFont labelFont(m_thumbFailed ? 26 : 11,
                             wxFONTFAMILY_DEFAULT,
                             wxFONTSTYLE_NORMAL,
                             wxFONTWEIGHT_NORMAL);
            gc->SetFont(labelFont, UIColors::TextMuted);

            double tw, th, td, tex;
            gc->GetTextExtent(label, &tw, &th, &td, &tex);
            gc->DrawText(label,
                         (W  - tw) / 2.0,
                         (thumbH - th) / 2.0);
        }
    }

    // ── Text area (bottom 28%) ────────────────────────────────────────────
    const double textY    = thumbH + 8.0;
    const double padX     = 8.0;
    const double textW    = W - padX * 2.0;
    double       cursorY  = textY;

    if (!m_title.IsEmpty()) {
        wxFont titleFont(10, wxFONTFAMILY_DEFAULT,
                         wxFONTSTYLE_NORMAL, wxFONTWEIGHT_SEMIBOLD);
        gc->SetFont(titleFont, UIColors::TextPrimary);
        cursorY = DrawWrappedText(gc, m_title, 2, padX, cursorY, textW);
        cursorY += 3.0;
    }

    if (!m_channelID.IsEmpty()) {
        wxFont chanFont(9, wxFONTFAMILY_DEFAULT,
                        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        gc->SetFont(chanFont, UIColors::TextSecondary);

        double tw, th, td, tex;
        gc->GetTextExtent(m_channelID, &tw, &th, &td, &tex);
        // Truncate with ellipsis if needed
        wxString chanLabel = m_channelID;
        if (tw > textW) {
            while (!chanLabel.IsEmpty()) {
                wxString cand = chanLabel + L"\u2026";
                gc->GetTextExtent(cand, &tw, &th, &td, &tex);
                if (tw <= textW) { chanLabel = cand; break; }
                chanLabel.RemoveLast();
            }
        }
        gc->DrawText(chanLabel, padX, cursorY);
    }

    // ── Hover border ─────────────────────────────────────────────────────
    if (m_hovered) {
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        wxPen accentPen(UIColors::Accent, 3);
        gc->SetPen(accentPen);
        gc->DrawRoundedRectangle(1.5, 1.5, W - 3.0, H - 3.0, 14.0);
    }

    delete gc;
}
