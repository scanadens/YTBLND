// ============================================================================
// VideoCard.cpp — Custom video preview card widget implementation
// ============================================================================

#include "VideoCard.hpp"

#include <algorithm>
#include <thread>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/utils.h>   // wxLaunchDefaultBrowser

#include <curl/curl.h>

#include "UIColors.hpp"

// ---------------------------------------------------------------------------
// Event definitions
// ---------------------------------------------------------------------------
wxDEFINE_EVENT(EVT_THUMBNAIL_LOADED,    ThumbnailEvent);
wxDEFINE_EVENT(EVT_CHANNEL_LOGO_LOADED, ThumbnailEvent);

// ---------------------------------------------------------------------------
// Internal curl helper — downloads a URL into memory and returns a wxImage.
// Called only from background threads.
// ---------------------------------------------------------------------------
namespace {

size_t curlWrite(void* ptr, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

wxImage fetchImageViaCurl(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return wxImage();

    std::string buf;
    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        15L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || buf.empty()) return wxImage();

    wxMemoryInputStream stream(buf.data(), buf.size());
    wxImage img;
    // wxBITMAP_TYPE_ANY lets wxWidgets detect JPEG / PNG / WebP automatically
    img.LoadFile(stream, wxBITMAP_TYPE_ANY);
    return img;
}

} // namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
VideoCard::VideoCard(wxWindow* parent, AppController& controller)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(260, 250))
    , m_controller(controller)
{
    SetBackgroundColour(UIColors::Surface);
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    Bind(wxEVT_PAINT,        &VideoCard::OnPaint,           this);
    Bind(wxEVT_ENTER_WINDOW, &VideoCard::OnMouseEnter,      this);
    Bind(wxEVT_LEAVE_WINDOW, &VideoCard::OnMouseLeave,      this);
    Bind(wxEVT_LEFT_UP,      &VideoCard::OnClick,           this);
    Bind(EVT_THUMBNAIL_LOADED,    &VideoCard::OnThumbnailLoaded,  this);
    Bind(EVT_CHANNEL_LOGO_LOADED, &VideoCard::OnChannelLogoLoaded, this);

    if (!wxImage::FindHandler(wxBITMAP_TYPE_JPEG))
        wxImage::AddHandler(new wxJPEGHandler());
    if (!wxImage::FindHandler(wxBITMAP_TYPE_PNG))
        wxImage::AddHandler(new wxPNGHandler());
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void VideoCard::SetVideo(const Video& video) {
    m_videoID       = wxString::FromUTF8(video.getVideoID());
    m_title         = wxString::FromUTF8(video.getTitle());
    m_channelID     = wxString::FromUTF8(video.getChannelID());
    m_channelName   = wxString::FromUTF8(video.getChannelName());
    m_thumbnailURL  = wxString::FromUTF8(video.getThumbnailURL());
    m_channelLogoURL= wxString::FromUTF8(video.getChannelLogoURL());

    m_thumbnail  = wxBitmap();
    m_thumbLoading = false; m_thumbFailed = false;

    m_channelLogo = wxBitmap();
    m_logoLoading = false; m_logoFailed = false;

    FetchThumbnailAsync();
    FetchChannelLogoAsync();
    Refresh();
}

void VideoCard::Clear() {
    m_videoID = m_title = m_channelID = m_channelName
              = m_thumbnailURL = m_channelLogoURL = wxEmptyString;

    m_thumbnail = wxBitmap();
    m_thumbLoading = m_thumbFailed = false;

    m_channelLogo = wxBitmap();
    m_logoLoading = m_logoFailed = false;

    Refresh();
}

// ---------------------------------------------------------------------------
// Async fetches
// ---------------------------------------------------------------------------
void VideoCard::FetchThumbnailAsync() {
    if (m_videoID.IsEmpty()) return;

    m_thumbLoading = true;
    ++m_thumbGen;

    // Build the URL: prefer the stored thumbnailURL, fall back to YouTube CDN
    std::string url;
    if (!m_thumbnailURL.IsEmpty()) {
        url = std::string(m_thumbnailURL.ToUTF8());
    } else {
        url = "https://img.youtube.com/vi/" +
              std::string(m_videoID.ToUTF8()) + "/hqdefault.jpg";
    }

    VideoCard* card = this;
    int        gen  = m_thumbGen;

    std::thread([url, card, gen]() {
        wxImage img = fetchImageViaCurl(url);
        bool    ok  = img.IsOk();
        auto* evt = new ThumbnailEvent(EVT_THUMBNAIL_LOADED, wxID_ANY,
                                       std::move(img), ok, gen);
        wxQueueEvent(card, evt);
    }).detach();
}

void VideoCard::FetchChannelLogoAsync() {
    if (m_channelLogoURL.IsEmpty()) return;

    m_logoLoading = true;
    ++m_logoGen;

    std::string url  = std::string(m_channelLogoURL.ToUTF8());
    VideoCard*  card = this;
    int         gen  = m_logoGen;

    std::thread([url, card, gen]() {
        wxImage img = fetchImageViaCurl(url);
        bool    ok  = img.IsOk();
        auto* evt = new ThumbnailEvent(EVT_CHANNEL_LOGO_LOADED, wxID_ANY,
                                       std::move(img), ok, gen);
        wxQueueEvent(card, evt);
    }).detach();
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------
void VideoCard::OnThumbnailLoaded(ThumbnailEvent& evt) {
    if (evt.generation != m_thumbGen) return;
    m_thumbLoading = false;
    if (evt.success && evt.image.IsOk()) {
        m_thumbnail   = wxBitmap(evt.image);
        m_thumbFailed = false;
    } else {
        m_thumbFailed = true;
    }
    Refresh();
}

void VideoCard::OnChannelLogoLoaded(ThumbnailEvent& evt) {
    if (evt.generation != m_logoGen) return;
    m_logoLoading = false;
    if (evt.success && evt.image.IsOk()) {
        m_channelLogo = wxBitmap(evt.image);
        m_logoFailed  = false;
    } else {
        m_logoFailed = true;
    }
    Refresh();
}

void VideoCard::OnMouseEnter(wxMouseEvent& /*evt*/) { m_hovered = true;  Refresh(); }
void VideoCard::OnMouseLeave(wxMouseEvent& /*evt*/) { m_hovered = false; Refresh(); }

void VideoCard::OnClick(wxMouseEvent& /*evt*/) {
    if (m_videoID.IsEmpty()) return;
    wxLaunchDefaultBrowser(
        wxString::Format("https://www.youtube.com/watch?v=%s", m_videoID));
    EventPayload payload;
    payload["videoID"] = std::string(m_videoID.ToUTF8());
    m_controller.getEventRouter().dispatch("playVideo", payload);
}

// ---------------------------------------------------------------------------
// Painting helpers
// ---------------------------------------------------------------------------
static double LineHeight(wxGraphicsContext* gc) {
    double tw, th, td, tex;
    gc->GetTextExtent("Wg", &tw, &th, &td, &tex);
    return th;
}

int VideoCard::DrawWrappedText(wxGraphicsContext* gc,
                               const wxString&    text,
                               int                maxLines,
                               double x, double y,
                               double maxWidth) const {
    if (text.IsEmpty()) return static_cast<int>(y);

    double tw, th, td, tex;
    gc->GetTextExtent("Wg", &tw, &th, &td, &tex);
    double lineH = th + 2.0;

    wxArrayString words = wxSplit(text, ' ');
    wxString currentLine;
    int      lineCount = 0;

    auto flush = [&](const wxString& line, bool isLast) {
        if (lineCount >= maxLines) return;
        wxString toDraw = line;
        gc->GetTextExtent(toDraw, &tw, &th, &td, &tex);
        if (!isLast || tw > maxWidth) {
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
    if (!currentLine.IsEmpty()) flush(currentLine, true);
done:
    return static_cast<int>(y + lineCount * lineH);
}

// ---------------------------------------------------------------------------
// OnPaint
// ---------------------------------------------------------------------------
void VideoCard::OnPaint(wxPaintEvent& /*evt*/) {
    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) return;

    const wxSize  sz = GetClientSize();
    const double  W  = sz.GetWidth();
    const double  H  = sz.GetHeight();

    // ── Card background ──────────────────────────────────────────────────
    gc->SetBrush(wxBrush(UIColors::Surface));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(0, 0, W, H, 10.0);

    // ── Thumbnail area — 16:9 capped at 145 px so the text area never
    //    gets squeezed out when the grid sizer stretches cards wider.
    const double thumbH = std::min(W * 9.0 / 16.0, 300.0);

    if (m_thumbnail.IsOk()) {
        wxImage scaled = m_thumbnail.ConvertToImage()
                                    .Scale(static_cast<int>(W),
                                           static_cast<int>(thumbH),
                                           wxIMAGE_QUALITY_BILINEAR);
        gc->DrawBitmap(wxBitmap(scaled), 0, 0, W, thumbH);
    } else {
        gc->SetBrush(wxBrush(UIColors::SurfaceRaised));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(0, 0, W, thumbH);

        wxString label;
        if      (m_thumbLoading) label = "Loading...";
        else if (m_thumbFailed)  label = L"\u25B6";
        // else: no video set — blank

        if (!label.IsEmpty()) {
            wxFont lf(m_thumbFailed ? 26 : 11, wxFONTFAMILY_DEFAULT,
                      wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            gc->SetFont(lf, UIColors::TextMuted);
            double tw, th, td, tex;
            gc->GetTextExtent(label, &tw, &th, &td, &tex);
            gc->DrawText(label, (W - tw) / 2.0, (thumbH - th) / 2.0);
        }
    }

    // ── Title ────────────────────────────────────────────────────────────
    const double padX   = 8.0;
    const double textW  = W - padX * 2.0;
    double       curY   = thumbH + 8.0;

    if (!m_title.IsEmpty()) {
        wxFont titleFont(10, wxFONTFAMILY_DEFAULT,
                         wxFONTSTYLE_NORMAL, wxFONTWEIGHT_SEMIBOLD);
        gc->SetFont(titleFont, UIColors::TextPrimary);
        curY = DrawWrappedText(gc, m_title, 2, padX, curY, textW);
        curY += 4.0;
    }

    // ── Channel row: round logo  +  channel name ─────────────────────────
    // Only render when we have at least a channel name or ID to show.
    // Avoids an orphaned grey circle for videos not yet enriched by the API.
    wxString chanLabel = m_channelName.IsEmpty() ? m_channelID : m_channelName;
    const bool hasChannelData = !chanLabel.IsEmpty() || m_channelLogo.IsOk()
                                || m_logoLoading;

    if (hasChannelData) {
        const double logoSize = 24.0;
        const double rowH     = logoSize;

        if (m_channelLogo.IsOk()) {
            // Punch a circle into the alpha channel — renders as a disc.
            int sz = static_cast<int>(logoSize);
            wxImage scaled = m_channelLogo.ConvertToImage()
                                          .Scale(sz, sz, wxIMAGE_QUALITY_BILINEAR);
            if (!scaled.HasAlpha()) scaled.InitAlpha();

            const double cx = sz / 2.0, cy = sz / 2.0, r = sz / 2.0;
            for (int py = 0; py < sz; ++py)
                for (int px = 0; px < sz; ++px) {
                    double dx = px + 0.5 - cx, dy = py + 0.5 - cy;
                    if (dx * dx + dy * dy > r * r)
                        scaled.SetAlpha(px, py, wxALPHA_TRANSPARENT);
                }
            gc->DrawBitmap(wxBitmap(scaled), padX, curY, logoSize, logoSize);
        } else {
            // Placeholder circle (shown while logo is loading)
            gc->SetBrush(wxBrush(UIColors::SurfaceRaised));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawEllipse(padX, curY, logoSize, logoSize);
        }

        if (!chanLabel.IsEmpty()) {
            wxFont chanFont(9, wxFONTFAMILY_DEFAULT,
                            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            gc->SetFont(chanFont, UIColors::TextSecondary);

            const double textX     = padX + logoSize + 5.0;
            const double chanTextW = W - textX - padX;

            double tw, th, td, tex;
            gc->GetTextExtent(chanLabel, &tw, &th, &td, &tex);
            if (tw > chanTextW) {
                while (!chanLabel.IsEmpty()) {
                    wxString cand = chanLabel + L"\u2026";
                    gc->GetTextExtent(cand, &tw, &th, &td, &tex);
                    if (tw <= chanTextW) { chanLabel = cand; break; }
                    chanLabel.RemoveLast();
                }
            }
            gc->DrawText(chanLabel, textX, curY + (rowH - th) / 2.0);
        }
    }

    // ── Hover border ─────────────────────────────────────────────────────
    if (m_hovered) {
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->SetPen(wxPen(UIColors::Accent, 3));
        gc->DrawRoundedRectangle(1.5, 1.5, W - 3.0, H - 3.0, 14.0);
    }

    delete gc;
}
