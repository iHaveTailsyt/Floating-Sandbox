/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "View.h"

#include <UILib/LocalizationManager.h>
#include <UILib/LoggingDialog.h>

#include <Game/ResourceLocator.h>

#include <GameOpenGL/GameOpenGL.h>

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/glcanvas.h> // Need to include this *after* our glad.h has been included,
 // so that wxGLCanvas ends up *not* including the system's OpenGL header but glad's instead
#include <wx/menu.h>
#include <wx/panel.h>

#include <memory>

namespace ShipBuilder {

/*
 * The main window of the ship builder GUI.
 */
class MainFrame final : public wxFrame
{
public:

    MainFrame(
        wxApp * mainApp,
        ResourceLocator const & resourceLocator,
        LocalizationManager const & localizationManager);

    void Open();

private:

    wxPanel * CreateFilePanel();
    wxPanel * CreateToolSettingsPanel();
    wxPanel * CreateGamePanel();
    wxPanel * CreateViewPanel();
    wxPanel * CreateToolbarPanel();
    wxPanel * CreateWorkPanel();

    void OnWorkCanvasResize(wxSizeEvent & event);
    void OnWorkCanvasLeftDown(wxMouseEvent & event);
    void OnWorkCanvasLeftUp(wxMouseEvent & event);
    void OnWorkCanvasRightDown(wxMouseEvent & event);
    void OnWorkCanvasRightUp(wxMouseEvent & event);
    void OnWorkCanvasMouseMove(wxMouseEvent & event);
    void OnWorkCanvasMouseWheel(wxMouseEvent & event);
    void OnQuit(wxCommandEvent & event);
    void OnOpenLogWindowMenuItemSelected(wxCommandEvent & event);

private:

    bool const mIsStandAlone;
    wxApp * const mMainApp;

    std::unique_ptr<View> mView;

    //
    // Helpers
    //

    ResourceLocator const & mResourceLocator;
    LocalizationManager const & mLocalizationManager;

    //
    // UI
    //

    wxPanel * mMainPanel;
    std::unique_ptr<wxGLCanvas> mWorkCanvas;
    std::unique_ptr<wxGLContext> mGLContext;
    bool mIsMouseCapturedByWorkCanvas;

    //
    // Dialogs
    //

    std::unique_ptr<LoggingDialog> mLoggingDialog;
};

}