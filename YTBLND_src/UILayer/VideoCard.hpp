/**
 * \file VideoCard.hpp
 * \brief Custom wxPanel widget that displays a single YouTube video preview.
 * \author Jasmine Kumar
 *
 * Layout (top -> bottom):
 * - 16:9 thumbnail - fetched asynchronously via libcurl from thumbnailURL,
 *   falling back to img.youtube.com/vi/\<id\>/hqdefault.jpg if the URL is empty.
 * - Video title (up to 2 wrapped lines).
 * - Channel row: 24 px round channel logo + channel name.
 *   The logo is fetched asynchronously from channelLogoURL.
 *
 * Both async fetches use a generation counter so stale results from an earlier
 * SetVideo() call are silently discarded.  Clicking the card opens the video
 * in the system browser and dispatches a \c "playVideo" event.
 */

#pragma once

#include <wx/wx.h>
#include <wx/graphics.h>

#include "../ModelLayer/Video.hpp"
#include "../AppLayer/AppController.hpp"

/**
 * \brief Event fired on the VideoCard when an async image fetch completes.
 *
 * Used for both the video thumbnail (EVT_THUMBNAIL_LOADED) and the channel
 * logo (EVT_CHANNEL_LOGO_LOADED).  The generation field must match the
 * card's current counter or the event is treated as stale and discarded.
 */
/**
 * \class ThumbnailEvent
 * \brief ThumbnailEvent class declaration.
 */
class ThumbnailEvent : public wxEvent {
public:
    wxImage image;      ///< Decoded image (valid only when success is true).
    bool    success;    ///< True if the fetch and decode both succeeded.
    int     generation; ///< Must match VideoCard::m_thumbGen or m_logoGen.

    ThumbnailEvent(wxEventType type, int id, wxImage img, bool ok, int gen)
        : wxEvent(id, type), image(std::move(img)), success(ok), generation(gen) {}

    wxEvent* Clone() const override { return new ThumbnailEvent(*this); }
};

wxDECLARE_EVENT(EVT_THUMBNAIL_LOADED,    ThumbnailEvent);  ///< Fired when the video thumbnail is ready.
wxDECLARE_EVENT(EVT_CHANNEL_LOGO_LOADED, ThumbnailEvent);  ///< Fired when the channel logo is ready.

/**
 * \brief Custom wxPanel widget that displays a single YouTube video preview.
 */
/**
 * \class VideoCard
 * \brief VideoCard class declaration.
 */
class VideoCard : public wxPanel {
public:
    /**
     * Constructs a VideoCard.
     * \param parent     Parent window.
     * \param controller Application controller used to dispatch playVideo events.
     */
    VideoCard(wxWindow* parent, AppController& controller);

    /**
     * Loads a video into the card and starts async image fetches.
     * \param video Video whose metadata and images should be displayed.
     */
    void SetVideo(const Video& video);

    /** Resets the card to its empty placeholder state. */
    void Clear();

    wxSize DoGetBestSize() const override { return wxSize(260, 250); }

private:
    AppController& m_controller;

    wxString m_videoID;
    wxString m_title;
    wxString m_channelID;
    wxString m_channelName;
    wxString m_thumbnailURL;
    wxString m_channelLogoURL;

    wxBitmap m_thumbnail;
    bool     m_thumbLoading = false;
    bool     m_thumbFailed  = false;
    int      m_thumbGen     = 0;     ///< Incremented on each SetVideo/Clear to discard stale fetches.

    wxBitmap m_channelLogo;
    bool     m_logoLoading  = false;
    bool     m_logoFailed   = false;
    int      m_logoGen      = 0;     ///< Incremented on each SetVideo/Clear to discard stale fetches.

    bool m_hovered = false;

    /** Spawns a background thread to fetch and decode the video thumbnail. */
    void FetchThumbnailAsync();
    /** Spawns a background thread to fetch and decode the channel logo. */
    void FetchChannelLogoAsync();

    void OnThumbnailLoaded  (ThumbnailEvent& evt);
    void OnChannelLogoLoaded(ThumbnailEvent& evt);
    void OnPaint     (wxPaintEvent&  evt);
    void OnMouseEnter(wxMouseEvent&  evt);
    void OnMouseLeave(wxMouseEvent&  evt);
    void OnClick     (wxMouseEvent&  evt);

    /**
     * Draws word-wrapped text with ellipsis trimming on the last line.
     * \param gc       Graphics context to draw into.
     * \param text     Text to render.
     * \param maxLines Maximum number of lines before truncating.
     * \param x        Left edge of the text area.
     * \param y        Top edge of the first line.
     * \param maxWidth Maximum pixel width of a line.
     * \return Y coordinate immediately below the last rendered line.
     */
    int DrawWrappedText(wxGraphicsContext* gc,
                        const wxString&    text,
                        int                maxLines,
                        double x, double y,
                        double maxWidth) const;
};
