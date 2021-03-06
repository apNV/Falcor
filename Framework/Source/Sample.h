/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once
#include "glm/glm.hpp"
#include <set>
#include <string>
#include <stdint.h>
#include "Core/Window.h"
#include "utils/FrameRate.h"
#include "utils/Gui.h"
#include "utils/TextRenderer.h"
#include "core/RenderContext.h"
#include "Utils/Video/VideoEncoderUI.h"

namespace Falcor
{
#ifdef _DEBUG
#define _SHOW_MB_BY_DEFAULT true
#else
#define _SHOW_MB_BY_DEFAULT false
#endif

    /** Sample configuration.
    */
    struct SampleConfig
    {
        Window::Desc windowDesc;            ///< Controls window and graphics device creation
        bool enableVsync = false;           ///< Controls vertical-sync
        bool showMessageBoxOnError = _SHOW_MB_BY_DEFAULT; ///< Show message box on framework/API errors.
        float timeScale = 1;                ///< A scaling factor for the time elapsed between frames.
        bool freezeTimeOnStartup = false;   ///< Control whether or not to start the clock when the sample start running.
    };

    /** Bootstrapper class for Falcor.
        User should create a class which inherits from CSample, then call CSample::Run() to start the sample.
        The render loop will then call the user's overridden callback functions.
    */
    class Sample : public Window::ICallbacks
    {
    public:
        Sample();
        virtual ~Sample();
        Sample(const Sample&) = delete;
        Sample& operator=(const Sample&) = delete;
        
        /** Entry-point to CSample().
            User should call this to start processing.
            \param Config Requested sample configuration
        */
        virtual void run(const SampleConfig& config) final;

    protected:
        // Callbacks
        /** Called once right after context creation.
        */
        virtual void onLoad() {}
        /** Called on each frame render.
        */
        virtual void onFrameRender() {}
        /** Called right before the context is destroyed
        */
        virtual void onShutdown() {}
        /** Called every time the swap-chain is resized. You can query the default FBO for the new size and sample count of the window
        */
        virtual void onResizeSwapChain() {}
        /** Called every time the user requests shader recompilation(by pressing F5)
        */
        virtual void onDataReload() {}
        /** Called every time a key event occurred
        \param keyEvent The keyboard event
        \return true if the event was consumed by the callback, otherwise false
        */
        virtual bool onKeyEvent(const KeyboardEvent& keyEvent) { return false; }
        /** Called every time a mouse event occurred
        \param mouseEvent The mouse event
        \return true if the event was consumed by the callback, otherwise false
        */
        virtual bool onMouseEvent(const MouseEvent& mouseEvent) { return false; }
        
        /** Resize the swap-chain buffers
            \param width Requested width
            \param height Requested height
        */
        void resizeSwapChain(uint32_t width, uint32_t height);

        /** Get whether the given key is pressed
            \param key The key
        */
        bool isKeyPressed(const KeyboardEvent::Key& key) const;

        const FrameRate& frameRate() const { return mFrameRate; }

        /** Render a text string
            \param str The string to render
            \param position Window position of the string (top-left corner)
            \param shadowOffset Offset for an outline shadow. Disabled if zero.
        */
        void renderText(const std::string& str, const glm::vec2& position, const glm::vec2 shadowOffset = glm::vec2(1.f, 1.f)) const;

        /** Get the global sample message, which includes the FPS and potentially help message
            \param includeHelpMsg If true, the returned message will contain the help message, otherwise will only contain the FPS.
        */
        const std::string getGlobalSampleMessage(bool includeHelpMsg) const;

        /** Close the window and exit the application
        */
        void shutdownApp();

        /** Poll for window events (useful when running long pieces of code)
        */
        void pollForEvents();
        
        /** Swap buffers
        */
        void swapBuffers();

        void setWindowTitle(std::string title);
        
        void toggleUI(bool showUI);
        Gui::UniquePtr mpGui;                             ///< Main sample GUI
        RenderContext::SharedPtr mpRenderContext;         ///< The rendering context
        Fbo::SharedPtr mpDefaultFBO;                      ///< The default FBO object
        bool mFreezeTime;                                 ///< Whether global time is frozen
        double mCurrentTime = 0;                          ///< Global time

    protected:
        enum class TextMode
        {
            All,
            FpsOnly,
            NoText,

            Count
        };

        void renderFrame() override;
        void handleFrameBufferSizeChange(const Fbo::SharedPtr& pFBO) override;
        void handleKeyboardEvent(const KeyboardEvent& keyEvent) override;
        void handleMouseEvent(const MouseEvent& mouseEvent) override;
        virtual float getTimeScale() final { return mTimeScale; }
        void initVideoCapture();
        void captureScreen();
        void setTextMode(TextMode mode);

    private:
        // Private functions
        void initUI();
        void printProfileData();
        void calculateTime();

        void startVideoCapture();
        void endVideoCapture();
        void captureVideoFrame();

        Window::UniquePtr mpWindow;
        bool mVsyncOn = false;

        bool mCaptureScreen = false;
        bool mShowUI = true;

        struct VideoCaptureData
        {
            VideoEncoderUI::UniquePtr pUI;
            VideoEncoder::UniquePtr pVideoCapture;
            uint8_t* pFrame = nullptr;
            float timeDelta;
        };

        VideoCaptureData mVideoCapture;

        FrameRate mFrameRate;
        float mTimeScale;
        TextMode mTextMode = TextMode::All;

        TextRenderer::UniquePtr mpTextRenderer;
        std::set<KeyboardEvent::Key> mPressedKeys;

        // GUI callbacks
        static void GUI_CALL captureScreenCB(void* pUserData);
        static void GUI_CALL initVideoCaptureCB(void* pUserData);
        static void GUI_CALL startVideoCaptureCB(void* pUserData);
        static void GUI_CALL endVideoCaptureCB(void* pUserData);
    };
};