#pragma once

#include <wx/wx.h>
#include <wx/graphics.h>

#include "../ModelLayer/Video.h"
#include "../AppLayer/AppController.h"

// ---------------------------------------------------------------------------
// Custom event: fired on the VideoCard when the async thumbnail fetch finishes
// ---------------------------------------------------------------------------
class ThumbnailEvent : public wxEvent {
public:
    wxImage image;
    bool    success;

    ThumbnailEvent(wxEventType type, int id, wxImage img, bool ok)
        : wxEvent(id, type), image(std::move(img)), success(ok) {}

    // Required by wxWidgets event system
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
