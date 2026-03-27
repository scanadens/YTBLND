// ============================================================================
// VideoCard.h — Custom video preview card widget
//
// A self-contained wxPanel that displays a single YouTube video:
//   • Top 72% — thumbnail fetched asynchronously from img.youtube.com
//   • Bottom 28% — video title (up to 2 lines) and channel ID
//   • Accent-coloured rounded border appears on hover
//   • Clicking opens the video in the user's default browser
//
// ASYNC THUMBNAIL LOADING
// ───────────────────────
// SetVideo() spawns a std::thread that downloads the JPEG thumbnail from
//   http://img.youtube.com/vi/<videoID>/hqdefault.jpg
// On completion the thread posts a ThumbnailEvent back to the UI thread via
// wxQueueEvent (thread-safe).  The card shows "Loading..." until the fetch
// completes, then either the image or a ▶ placeholder on failure.
//
// THREAD SAFETY NOTE
// ──────────────────
// The detached thread holds a raw VideoCard* pointer.  wxQueueEvent will
// silently discard the event if the card has been destroyed, which is safe.
// However, if SetVideo() is called again before the previous fetch finishes,
// the stale event will still arrive — OnThumbnailLoaded() will overwrite
// whatever the second fetch loaded.
// TODO: Add a generation counter (or cancel flag) so stale thumbnail events
//       from earlier fetches are ignored when the card has been reused.
//
// CLICK BEHAVIOUR
// ───────────────
// OnClick dispatches "playVideo" to AppController and opens the YouTube URL
// in the system browser.
// TODO: If in-app playback is ever added, dispatch to a video player panel
//       instead of opening the browser.
//
// TITLE / CHANNEL DATA
// ────────────────────
// Video::getTitle() and getChannelID() may return empty strings if the CSV
// only contained a video ID.  VideoCard omits those text areas gracefully.
// TODO: Fetch richer metadata (actual channel name, view count, etc.) via the
//       YouTube Data API v3 when an API key is available.
// ============================================================================

#pragma once

#include <wx/wx.h>
#include <wx/graphics.h>

#include "../ModelLayer/Video.h"
#include "../AppLayer/AppController.hpp"

// ---------------------------------------------------------------------------
// Custom event: fired on the VideoCard when the async thumbnail fetch finishes
// ---------------------------------------------------------------------------
class ThumbnailEvent : public wxEvent {
public:
    wxImage image;
    bool    success;
    int     generation; // must match VideoCard::m_generation or the event is stale

    ThumbnailEvent(wxEventType type, int id, wxImage img, bool ok, int gen)
        : wxEvent(id, type), image(std::move(img)), success(ok), generation(gen) {}

    wxEvent* Clone() const override { return new ThumbnailEvent(*this); }
};

wxDECLARE_EVENT(EVT_THUMBNAIL_LOADED, ThumbnailEvent);

// ---------------------------------------------------------------------------
// VideoCard
// ---------------------------------------------------------------------------
class VideoCard : public wxPanel {
public:
    VideoCard(wxWindow* parent, AppController& controller);

    // Display a specific video (triggers async thumbnail fetch)
    void SetVideo(const Video& video);

    // Return to placeholder / empty state
    void Clear();

    wxSize DoGetBestSize() const override { return wxSize(280, 220); }

private:
    AppController& m_controller;

    // Current video data (empty strings when in placeholder state)
    wxString m_videoID;
    wxString m_title;
    wxString m_channelID;

    // Thumbnail state
    wxBitmap m_thumbnail;       // valid when loaded successfully
    bool     m_thumbLoading;    // true while the fetch thread is running
    bool     m_thumbFailed;     // true when fetch finished but failed
    int      m_generation;      // incremented on each SetVideo/Clear to discard stale events

    // Hover / interaction state
    bool m_hovered;

    // --- helpers ---
    void FetchThumbnailAsync();
    void OnThumbnailLoaded(ThumbnailEvent& evt);
    void OnPaint(wxPaintEvent& evt);
    void OnMouseEnter(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);
    void OnClick(wxMouseEvent& evt);

    // Draw wrapped text; returns the y-coordinate after the last line
    int DrawWrappedText(wxGraphicsContext* gc,
                        const wxString&    text,
                        int maxLines,
                        double x, double y,
                        double maxWidth) const;
};
