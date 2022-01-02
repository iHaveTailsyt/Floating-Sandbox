/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"
#include "IUserInterface.h"
#include "MaterialPalette.h"
#include "ModelValidationDialog.h"
#include "ResizeDialog.h"
#include "ShipPropertiesEditDialog.h"
#include "StatusBar.h"
#include "View.h"
#include "WorkbenchState.h"

#include <UILib/BitmapButton.h>
#include <UILib/BitmapToggleButton.h>
#include <UILib/LocalizationManager.h>
#include <UILib/LoggingDialog.h>
#include <UILib/ShipLoadDialog.h>
#include <UILib/ShipSaveDialog.h>

#include <Game/MaterialDatabase.h>
#include <Game/ResourceLocator.h>
#include <Game/ShipTexturizer.h>

#include <GameOpenGL/GameOpenGL.h>

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/glcanvas.h> // Need to include this *after* our glad.h has been included, so that wxGLCanvas ends
                         // up *not* including the system's OpenGL header but glad's instead
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/scrolbar.h>
#include <wx/scrolwin.h>
#include <wx/slider.h>
#include <wx/statbmp.h>

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace ShipBuilder {

/*
 * The main window of the ship builder GUI.
 *
 * - Owns Controller and View
 * - Very thin, calls into Controller for each high-level interaction (e.g. new tool selected, tool setting changed) and for each mouse event
 * - Implements IUserInterface with interface needed by Controller, e.g. to make UI state changes, to capture the mouse, to update visualization of undo stack
 * - Owns WorkbenchState
 * - Implements ship load/save, giving/getting whole ShipDefinition to/from ModelController
 */
class MainFrame final : public wxFrame, public IUserInterface
{
public:

    MainFrame(
        wxApp * mainApp,
        wxIcon const & icon,
        ResourceLocator const & resourceLocator,
        LocalizationManager const & localizationManager,
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer,
        std::function<void(std::optional<std::filesystem::path>)> returnToGameFunctor);

    void OpenForNewShip();

    void OpenForLoadShip(std::filesystem::path const & shipFilePath);

public:

    //
    // IUserInterface
    //

    void RefreshView() override;

    void OnViewModelChanged() override;

    void OnShipNameChanged(std::string const & newName) override;

    void OnShipSizeChanged(ShipSpaceSize const & shipSize) override;

    void OnLayerPresenceChanged() override;

    void OnPrimaryVisualizationChanged(VisualizationType primaryVisualization) override;

    void OnGameVisualizationModeChanged(GameVisualizationModeType mode) override;
    void OnStructuralLayerVisualizationModeChanged(StructuralLayerVisualizationModeType mode) override;
    void OnElectricalLayerVisualizationModeChanged(ElectricalLayerVisualizationModeType mode) override;
    void OnRopesLayerVisualizationModeChanged(RopesLayerVisualizationModeType mode) override;
    void OnTextureLayerVisualizationModeChanged(TextureLayerVisualizationModeType mode) override;

    void OnModelDirtyChanged() override;

    void OnWorkbenchStateChanged() override;

    void OnCurrentToolChanged(std::optional<ToolType> tool) override;

    void OnUndoStackStateChanged() override;

    void OnToolCoordinatesChanged(std::optional<ShipSpaceCoordinates> coordinates) override;

    void OnError(wxString const & errorMessage) const override;

    ShipSpaceCoordinates GetMouseCoordinates() const override;

    std::optional<ShipSpaceCoordinates> GetMouseCoordinatesIfInWorkCanvas() const override;

    void SetToolCursor(wxImage const & cursorImage) override;

    void ResetToolCursor() override;

    void ScrollIntoViewIfNeeded(DisplayLogicalCoordinates const & workCanvasDisplayLogicalCoordinates) override;

private:

    wxAcceleratorEntry MakePlainAcceleratorKey(int key, wxMenuItem * menuItem);
    wxPanel * CreateFilePanel(wxWindow * parent);
    wxPanel * CreateShipSettingsPanel(wxWindow * parent);
    wxPanel * CreateToolSettingsPanel(wxWindow * parent);
    wxPanel * CreateVisualizationsPanel(wxWindow * parent);
    wxPanel * CreateVisualizationDetailsPanel(wxWindow * parent);
    wxPanel * CreateToolbarPanel(wxWindow * parent);
    wxPanel * CreateUndoPanel(wxWindow * parent);
    wxPanel * CreateWorkPanel(wxWindow * parent);

    std::tuple<wxPanel *, wxSizer *> CreateToolSettingsToolSizePanel(
        wxWindow * parent,
        wxString const & label,
        wxString const & tooltip,
        std::uint32_t minValue,
        std::uint32_t maxValue,
        std::uint32_t currentValue,
        std::function<void(std::uint32_t)> onValue);

    void OnWorkCanvasPaint(wxPaintEvent & event);
    void OnWorkCanvasResize(wxSizeEvent & event);
    void OnWorkCanvasLeftDown(wxMouseEvent & event);
    void OnWorkCanvasLeftUp(wxMouseEvent & event);
    void OnWorkCanvasRightDown(wxMouseEvent & event);
    void OnWorkCanvasRightUp(wxMouseEvent & event);
    void OnWorkCanvasMouseMove(wxMouseEvent & event);
    void OnWorkCanvasMouseWheel(wxMouseEvent & event);
    void OnWorkCanvasCaptureMouseLost(wxMouseCaptureLostEvent & event);
    void OnWorkCanvasMouseLeftWindow(wxMouseEvent & event);
    void OnWorkCanvasMouseEnteredWindow(wxMouseEvent & event);
    void OnWorkCanvasKeyDown(wxKeyEvent & event);
    void OnWorkCanvasKeyUp(wxKeyEvent & event);

    void OnNewShip(wxCommandEvent & event);
    void OnLoadShip(wxCommandEvent & event);
    void OnSaveShipMenuItem(wxCommandEvent & event);
    void OnSaveShip();
    void OnSaveShipAsMenuItem(wxCommandEvent & event);
    void OnSaveShipAs();
    void OnSaveAndGoBackMenuItem(wxCommandEvent & event);
    void OnSaveAndGoBack();
    void OnQuitAndGoBack(wxCommandEvent & event);
    void OnQuit(wxCommandEvent & event);
    void OnClose(wxCloseEvent & event);
    void OnEditUndoMenuItem(wxCommandEvent & event);
    void OnEditAutoTrimMenuItem(wxCommandEvent & event);
    void OnEditFlipHorizontallyMenuItem(wxCommandEvent & event);
    void OnEditFlipVerticallyMenuItem(wxCommandEvent & event);
    void OnEditResizeShipMenuItem(wxCommandEvent & event);
    void OnEditShipPropertiesMenuItem(wxCommandEvent & event);
    void OnZoomIn(wxCommandEvent & event);
    void OnZoomOut(wxCommandEvent & event);
    void OnResetView(wxCommandEvent & event);
    void OnOpenLogWindowMenuItemSelected(wxCommandEvent & event);
    void OnStructuralMaterialSelected(fsStructuralMaterialSelectedEvent & event);
    void OnElectricalMaterialSelected(fsElectricalMaterialSelectedEvent & event);
    void OnRopeMaterialSelected(fsStructuralMaterialSelectedEvent & event);

private:

    bool IsStandAlone() const
    {
        return !mReturnToGameFunctor;
    }

    void OnOpenForLoadShipIdleEvent(wxIdleEvent & event);

    void Open();

    void NewShip();

    void LoadShip();

    bool PreSaveShipCheck();

    bool SaveShip();

    bool SaveShipAs();

    void SaveAndSwitchBackToGame();

    void QuitAndSwitchBackToGame();

    void SwitchBackToGame(std::optional<std::filesystem::path> shipFilePath);

    void ImportTextureLayerFromImage();

    void OpenShipCanvasResize();

    void OpenShipProperties();

    void ValidateShip();

    void OpenMaterialPalette(
        wxMouseEvent const & event,
        LayerType layer,
        MaterialPlaneType plane);

    bool AskUserIfSure(wxString caption);

    int AskUserIfSave();

    bool AskUserIfRename(std::string const & newFilename);

    void ShowError(wxString const & message);

    void DoNewShip();

    bool DoLoadShip(std::filesystem::path const & shipFilePath);

    void DoSaveShip(std::filesystem::path const & shipFilePath);

    DisplayLogicalSize GetWorkCanvasSize() const;

    void RecalculateWorkCanvasPanning();

    void SetFrameTitle(std::string const & shipName, bool isDirty);

    void DeviateFocus();

    float OtherVisualizationsOpacitySliderToOpacity(int sliderValue) const;

    int OtherVisualizationsOpacityToSlider(float opacityValue) const;

    size_t LayerToVisualizationIndex(LayerType layer) const;

    //
    // UI Consistency
    //

    void ReconciliateUI();

    void ReconciliateUIWithViewModel();

    void ReconciliateUIWithShipName(std::string const & shipName);

    void ReconciliateUIWithShipSize(ShipSpaceSize const & shipSize);

    void ReconciliateUIWithLayerPresence();

    void ReconciliateUIWithPrimaryVisualizationSelection(VisualizationType primaryVisualization);

    void ReconciliateUIWithGameVisualizationModeSelection(GameVisualizationModeType mode);
    void ReconciliateUIWithStructuralLayerVisualizationModeSelection(StructuralLayerVisualizationModeType mode);
    void ReconciliateUIWithElectricalLayerVisualizationModeSelection(ElectricalLayerVisualizationModeType mode);
    void ReconciliateUIWithRopesLayerVisualizationModeSelection(RopesLayerVisualizationModeType mode);
    void ReconciliateUIWithTextureLayerVisualizationModeSelection(TextureLayerVisualizationModeType mode);

    void ReconciliateUIWithModelDirtiness();

    void ReconciliateUIWithWorkbenchState();

    void ReconciliateUIWithSelectedTool(std::optional<ToolType> tool);

    void ReconciliateUIWithUndoStackState();

private:

    wxApp * const mMainApp;

    std::function<void(std::optional<std::filesystem::path>)> const mReturnToGameFunctor;

    //
    // Owned members
    //

    std::unique_ptr<View> mView;
    std::unique_ptr<Controller> mController;

    //
    // Helpers
    //

    ResourceLocator const & mResourceLocator;
    LocalizationManager const & mLocalizationManager;
    MaterialDatabase const & mMaterialDatabase;
    ShipTexturizer const & mShipTexturizer;

    //
    // UI
    //

    wxPanel * mMainPanel;

    // Menu
    wxMenuItem * mSaveShipMenuItem;
    wxMenuItem * mSaveAndGoBackMenuItem;
    wxMenuItem * mUndoMenuItem;

    // File panel
    BitmapButton * mSaveShipButton;

    // Tool settings panel
    wxSizer * mToolSettingsPanelsSizer;
    std::vector<std::tuple<ToolType, wxPanel *>> mToolSettingsPanels;

    // Visualization panel
    std::array<BitmapToggleButton *, VisualizationCount> mVisualizationSelectButtons;
    std::array<BitmapButton *, LayerCount> mLayerExportButtons;
    std::array<BitmapButton *, LayerCount> mLayerDeleteButtons;

    // Visualization details panel
    wxSlider * mOtherVisualizationsOpacitySlider;
    wxSizer * mVisualizationModePanelsSizer;
    std::array<wxPanel *, VisualizationCount> mVisualizationModePanels;
    BitmapToggleButton * mGameVisualizationNoneModeButton;
    BitmapToggleButton * mGameVisualizationAutoTexturizationModeButton;
    BitmapToggleButton * mGameVisualizationTextureModeButton;
    BitmapToggleButton * mStructuralLayerVisualizationNoneModeButton;
    BitmapToggleButton * mStructuralLayerVisualizationMeshModeButton;
    BitmapToggleButton * mStructuralLayerVisualizationPixelModeButton;
    BitmapToggleButton * mElectricalLayerVisualizationNoneModeButton;
    BitmapToggleButton * mElectricalLayerVisualizationPixelModeButton;
    BitmapToggleButton * mRopesLayerVisualizationNoneModeButton;
    BitmapToggleButton * mRopesLayerVisualizationLinesModeButton;
    BitmapToggleButton * mTextureLayerVisualizationNoneModeButton;
    BitmapToggleButton * mTextureLayerVisualizationMatteModeButton;

    // Toolbar panel
    wxSizer * mToolbarPanelsSizer;
    std::array<wxPanel *, LayerCount> mToolbarPanels;
    std::array<BitmapToggleButton *, static_cast<size_t>(ToolType::_Last) + 1> mToolButtons;
    wxStaticBitmap * mStructuralForegroundMaterialSelector;
    wxStaticBitmap * mStructuralBackgroundMaterialSelector;
    wxStaticBitmap * mElectricalForegroundMaterialSelector;
    wxStaticBitmap * mElectricalBackgroundMaterialSelector;
    wxStaticBitmap * mRopesForegroundMaterialSelector;
    wxStaticBitmap * mRopesBackgroundMaterialSelector;
    wxBitmap mNullMaterialBitmap;

    // Undo stack panel
    wxScrolledWindow * mUndoStackPanel;

    // Work panel
    std::unique_ptr<wxGLCanvas> mWorkCanvas;
    std::unique_ptr<wxGLContext> mGLContext;
    wxScrollBar * mWorkCanvasHScrollBar;
    wxScrollBar * mWorkCanvasVScrollBar;

    // Misc UI elements
    std::unique_ptr<MaterialPalette<LayerType::Structural>> mStructuralMaterialPalette;
    std::unique_ptr<MaterialPalette<LayerType::Electrical>> mElectricalMaterialPalette;
    std::unique_ptr<MaterialPalette<LayerType::Ropes>> mRopesMaterialPalette;
    StatusBar * mStatusBar;

    //
    // Dialogs
    //

    std::unique_ptr<ShipLoadDialog> mShipLoadDialog;
    std::unique_ptr<ShipSaveDialog> mShipSaveDialog;
    std::unique_ptr<LoggingDialog> mLoggingDialog;
    std::unique_ptr<ResizeDialog> mResizeDialog;
    std::unique_ptr<ShipPropertiesEditDialog> mShipPropertiesEditDialog;
    std::unique_ptr<ModelValidationDialog> mModelValidationDialog;

    //
    // UI state
    //

    bool mutable mIsMouseInWorkCanvas;
    bool mutable mIsMouseCapturedByWorkCanvas;

    //
    // State
    //

    WorkbenchState mWorkbenchState;

    std::filesystem::path mOpenForLoadShipFilePath;
    std::optional<std::filesystem::path> mCurrentShipFilePath;
    std::vector<std::filesystem::path> mShipLoadDirectories;
};

}