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
#include "ShaderBuffers.h"

void ShaderBuffersSample::initUI()
{
    Gui::setGlobalHelpMessage("Sample application that shows how to use uniform-buffers");

    mpGui->addDir3FVar("Light Direction", &mLightData.worldDir);
    mpGui->addRgbColor("Light intensity", &mLightData.intensity);
    mpGui->addRgbColor("Surface Color", &mSurfaceColor);
    mpGui->addCheckBox("Count FS invocations", &mCountPixelShaderInvocations);
}

Vao::SharedConstPtr ShaderBuffersSample::getVao()
{
    auto pMesh = mpModel->getMesh(0);
    auto pVao = pMesh->getVao();
    return pVao;
}

void ShaderBuffersSample::onLoad()
{
    mpCamera = Camera::create();

    initUI();
    // create the program
    mpProgram = Program::createFromFile("ShaderBuffers.vs", "ShaderBuffers.fs");

    // Load the model
    mpModel = Model::createFromFile("teapot.obj", 0);

    // Plane has only one mesh, get the VAO now
    mpVao = getVao();
    auto pMesh = mpModel->getMesh(0);
    mIndexCount = pMesh->getIndexCount();

    // Initialize uniform-buffers data
    mLightData.intensity = glm::vec3(1, 1, 1);
    mLightData.worldDir = glm::vec3(0, -1, 0);

    // Set camera parameters
    glm::vec3 center = mpModel->getCenter();
    float radius = mpModel->getRadius();

    float nearZ = 0.1f;
    float farZ = radius * 10;
    mpCamera->setDepthRange(nearZ, farZ);

    // Initialize the camera controller
    mCameraController.attachCamera(mpCamera);
    mCameraController.setModelParams(center, radius, radius * 10);

    // create the uniform buffers
    auto pActiveVersion = mpProgram->getActiveProgramVersion().get();
    mpPerFrameCB = UniformBuffer::create(pActiveVersion, "PerFrameCB");
    mpLightCB = UniformBuffer::create(pActiveVersion, "LightCB");
    mpPixelCountBuffer = ShaderStorageBuffer::create(pActiveVersion, "PixelCount");

    // create rasterizer state
    RasterizerState::Desc rsDesc;
    rsDesc.setCullMode(RasterizerState::CullMode::Back);
    mpBackFaceCullRS = RasterizerState::create(rsDesc);

    // Depth test
    DepthStencilState::Desc dsDesc;
    dsDesc.setDepthTest(true);
    mpDepthTestDS = DepthStencilState::create(dsDesc);

}

void ShaderBuffersSample::onFrameRender()
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    mpDefaultFBO->clear(clearColor, 1.0f, 0, FboAttachmentType::All);

    mpRenderContext->setDepthStencilState(mpDepthTestDS, 0);
    mpRenderContext->setRasterizerState(mpBackFaceCullRS);

    mCameraController.update();

    // Update uniform-buffers data
    mpPerFrameCB->setVariable("m.worldMat", glm::mat4());
    mpPerFrameCB->setVariable("m.wvpMat", mpCamera->getProjMatrix() * mpCamera->getViewMatrix());
    mpPerFrameCB->setVariable("surfaceColor", mSurfaceColor);

    mpPerFrameCB->uploadToGPU();

    mpLightCB->setVariable("worldDir", mLightData.worldDir);
    mpLightCB->setVariable("intensity", mLightData.intensity);
    
    mpLightCB->uploadToGPU();

    // Set uniform buffers
    mpRenderContext->setProgram(mpProgram->getActiveProgramVersion());
    mpRenderContext->setShaderStorageBuffer(0, mpPixelCountBuffer);

    // Just for the sake of the example, we fetch the buffer location from the program here. We could have cached it, or better yet, just use "layout(binding = <someindex>" in the shader
    uint32_t bufferLocation = mpProgram->getUniformBufferBinding("PerFrameCB");
    mpRenderContext->setUniformBuffer(bufferLocation, mpPerFrameCB);
    bufferLocation = mpProgram->getUniformBufferBinding("LightCB");
    mpRenderContext->setUniformBuffer(bufferLocation, mpLightCB);

    mpRenderContext->setVao(mpVao);
    mpRenderContext->setTopology(RenderContext::Topology::TriangleList);
    mpRenderContext->drawIndexed(mIndexCount, 0, 0);

    std::string Txt = getGlobalSampleMessage(true) + '\n';
    if(mCountPixelShaderInvocations)
    {
#ifndef FALCOR_DX11
        mpPixelCountBuffer->readFromGPU();
        uint32_t FsInvocations;
        mpPixelCountBuffer->getVariable("count", FsInvocations);
        Txt += "FS was invoked " + std::to_string(FsInvocations) + " times.";
        mpPixelCountBuffer->setVariable("count", 0U);
        mpPixelCountBuffer->uploadToGPU();
#endif
    }
    renderText(Txt, glm::vec2(10, 10));
}

void ShaderBuffersSample::onShutdown()
{
}

void ShaderBuffersSample::onDataReload()
{
    mpVao = getVao();
}

bool ShaderBuffersSample::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mCameraController.onKeyEvent(keyEvent);
}

bool ShaderBuffersSample::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mCameraController.onMouseEvent(mouseEvent);
}

void ShaderBuffersSample::onResizeSwapChain()
{
    RenderContext::Viewport VP;
    VP.height = (float)mpDefaultFBO->getHeight();
    VP.width = (float)mpDefaultFBO->getWidth();
    mpRenderContext->setViewport(0, VP);

    mpCamera->setFovY(float(M_PI / 8));
    mpCamera->setAspectRatio(VP.width / VP.height);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    ShaderBuffersSample buffersSample;
    SampleConfig config;
    config.windowDesc.title = "Falcor Project Template";
    buffersSample.run(config);
}
