#include "Renderer.h"

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/imgui_impl_dx11.h"

#include <algorithm>

// For the DirectX Math library
using namespace DirectX;

Renderer::Renderer(
	Microsoft::WRL::ComPtr<ID3D11Device> device, 
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain, 
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, 
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV, 
	unsigned int width, 
	unsigned int height, 
	Sky* sky, 
	const std::vector<GameEntity*>& entities, 
	const std::vector<Light>& lights,
	int& lightCount,
	Mesh* lightMesh,
	SimpleVertexShader* lightVS,
	SimplePixelShader* lightPS,
	SimplePixelShader* PBRShader,
	SimpleVertexShader* fullscreenVS,
	SimplePixelShader* solidColorPS,
	SimplePixelShader* simpleTexturePS,
	SimplePixelShader* refractionPS) :
		device(device),
		context(context),
		swapChain(swapChain),
		backBufferRTV(backBufferRTV),
		depthBufferDSV(depthBufferDSV),
		windowWidth(width),
		windowHeight(height),
		sky(sky),
		entities(entities),
		lights(lights),
		lightCount(lightCount),
		lightMesh(lightMesh), 
		lightVS(lightVS),
		lightPS(lightPS),
		refractionScale(0.1f),
		useRefractionSilhouette(true),
		refractionFromNormalMap(true),
		indexOfRefraction(0.5f),
		fullscreenVS(fullscreenVS),
		solidColorPS(solidColorPS),
		simpleTexturePS(simpleTexturePS),
		refractionPS(refractionPS) {

	// initialize structs
	vsPerFrameData = {};
	psPerFrameData = {};

	D3D11_BUFFER_DESC bufferDesc = {};
	const SimpleConstantBuffer* scb = 0;

	// make a new vs per frame buffer
	scb = lightVS->GetBufferInfo("perFrame");
	scb->ConstantBuffer.Get()->GetDesc(&bufferDesc);
	device->CreateBuffer(&bufferDesc, 0, vsPerFrameConstantBuffer.GetAddressOf());

	// make a new ps per frame buffer
	scb = PBRShader->GetBufferInfo("perFrame");
	scb->ConstantBuffer.Get()->GetDesc(&bufferDesc);
	device->CreateBuffer(&bufferDesc, 0, psPerFrameConstantBuffer.GetAddressOf());

	// Create render targets
	CreateRenderTarget(windowWidth, windowHeight, sceneColorsRTV, sceneColorsSRV);
	CreateRenderTarget(windowWidth, windowHeight, sceneNormalsRTV, sceneNormalsSRV);
	CreateRenderTarget(windowWidth, windowHeight, sceneDepthsRTV, sceneDepthsSRV);
	CreateRenderTarget(windowWidth, windowHeight, silhouetteRTV, silhouetteSRV);

	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = true;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	device->CreateDepthStencilState(&depthDesc, refractionSilhouetteDepthState.GetAddressOf());
}

Renderer::~Renderer() {}

void Renderer::PreResize()
{
	backBufferRTV.Reset();
	depthBufferDSV.Reset();
}

void Renderer::PostResize(
	unsigned int width, 
	unsigned int height, 
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _backBufferRTV, 
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depthBufferDSV)
{
	windowWidth = width;
	windowHeight = height;
	backBufferRTV = _backBufferRTV;
	depthBufferDSV = _depthBufferDSV;

	sceneColorsRTV.Reset();
	sceneNormalsRTV.Reset();
	sceneDepthsRTV.Reset();
	sceneColorsSRV.Reset();
	sceneNormalsSRV.Reset();
	sceneDepthsSRV.Reset();

	// Recreate using the new window size
	CreateRenderTarget(windowWidth, windowHeight, sceneColorsRTV, sceneColorsSRV);
	CreateRenderTarget(windowWidth, windowHeight, sceneNormalsRTV, sceneNormalsSRV);
	CreateRenderTarget(windowWidth, windowHeight, sceneDepthsRTV, sceneDepthsSRV);
	CreateRenderTarget(windowWidth, windowHeight, silhouetteRTV, silhouetteSRV);
}

void Renderer::Render(Camera* camera)
{
	// Background color for clearing
	const float color[4] = { 0, 0, 0, 1 };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearRenderTargetView(sceneColorsRTV.Get(), color);
	context->ClearRenderTargetView(sceneNormalsRTV.Get(), color);
	context->ClearRenderTargetView(sceneDepthsRTV.Get(), color);
	context->ClearDepthStencilView(
		depthBufferDSV.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	ID3D11RenderTargetView* renderTargets[3] = {};
	renderTargets[0] = sceneColorsRTV.Get();
	renderTargets[1] = sceneNormalsRTV.Get();
	renderTargets[2] = sceneDepthsRTV.Get();

	context->OMSetRenderTargets(3, renderTargets, depthBufferDSV.Get());

	// collect per frame data
	{
		vsPerFrameData.ViewMatrix = camera->GetView();
		vsPerFrameData.ProjectionMatrix = camera->GetProjection();
		context->UpdateSubresource(vsPerFrameConstantBuffer.Get(), 0, 0, &vsPerFrameData, 0, 0);

		memcpy(&psPerFrameData.Lights, &lights[0], sizeof(Light) * lightCount);
		psPerFrameData.LightCount = lightCount;
		psPerFrameData.CameraPosition = camera->GetTransform()->GetPosition();
		psPerFrameData.SpecIBLTotalMipLevels = sky->GetMipLevels();
		context->UpdateSubresource(psPerFrameConstantBuffer.Get(), 0, 0, &psPerFrameData, 0, 0);
	}

	// sort entities by material
	std::vector<GameEntity*> toDraw(entities);
	std::sort(toDraw.begin(), toDraw.end(), [](const auto& e1, const auto& e2) {
		return e1->GetMaterial() < e2->GetMaterial();
		});

	// Collect all refractive entities for later
	std::vector<GameEntity*> refractiveEntities;

	// draw entities
	SimpleVertexShader* currentVS = 0;
	SimplePixelShader* currentPS = 0;
	Material* currentMaterial = 0;
	Mesh* currentMesh = 0;
	for (auto ge : toDraw) {
		// skip refractive materials
		if (ge->GetMaterial()->IsRefractive())
		{
			refractiveEntities.push_back(ge);
			continue;
		}

		// track current material
		if (currentMaterial != ge->GetMaterial()) {
			currentMaterial = ge->GetMaterial();

			// swap vertex shader if necessary
			if (currentVS != currentMaterial->GetVS()) {
				currentVS = currentMaterial->GetVS();
				currentVS->SetShader();

				context->VSSetConstantBuffers(0, 1, vsPerFrameConstantBuffer.GetAddressOf());
			}

			// swap pixel shader if necessary
			if (currentPS != currentMaterial->GetPS()) {
				currentPS = currentMaterial->GetPS();
				currentPS->SetShaderResourceView("BrdfLookUpMap", sky->GetBRDFLookUpTexture());
				currentPS->SetShaderResourceView("IrradianceIBLMap", sky->GetIrradianceMap());
				currentPS->SetShaderResourceView("SpecularIBLMap", sky->GetConvolvedSpecularMap());
				currentPS->SetShader();

				context->PSSetConstantBuffers(0, 1, psPerFrameConstantBuffer.GetAddressOf());
			}

			currentMaterial->SetPerMaterialDataAndResources(true);
		}

		if (currentMesh != ge->GetMesh()) {
			currentMesh = ge->GetMesh();

			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, currentMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(currentMesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		}

		if (currentVS != 0) {
			Transform* trans = ge->GetTransform();
			currentVS->SetMatrix4x4("world", trans->GetWorldMatrix());
			currentVS->SetMatrix4x4("worldInverseTranspose", trans->GetWorldInverseTransposeMatrix());
			currentVS->CopyBufferData("perObject");
		}

		if (currentMesh != 0) {
			context->DrawIndexed(currentMesh->GetIndexCount(), 0, 0);
		}
	}

	// Draw the light sources
	DrawPointLights(camera);

	// Draw the sky
	sky->Draw(camera);

	fullscreenVS->SetShader();

	renderTargets[0] = backBufferRTV.Get();
	context->OMSetRenderTargets(1, renderTargets, 0);
	simpleTexturePS->SetShader();
	simpleTexturePS->SetShaderResourceView("Pixels", sceneColorsSRV);
	context->Draw(3, 0);

	// Loop and render the refractive objects to the silhouette texture (if use silhouettes)
	if (useRefractionSilhouette)
	{
		renderTargets[0] = silhouetteRTV.Get();
		context->OMSetRenderTargets(1, renderTargets, depthBufferDSV.Get());

		// Depth state
		context->OMSetDepthStencilState(refractionSilhouetteDepthState.Get(), 0);

		// Loop and draw each one
		for (auto ge : refractiveEntities)
		{
			// Get this material and sub the refraction PS for now
			Material* mat = ge->GetMaterial();
			SimplePixelShader* prevPS = mat->GetPS();
			mat->SetPS(solidColorPS);

			// Overall material prep
			mat->PrepareMaterial(ge->GetTransform(), camera);
			mat->SetPerMaterialDataAndResources(true);

			// Set up the refraction specific data
			solidColorPS->SetFloat3("Color", XMFLOAT3(1, 1, 1));
			solidColorPS->CopyBufferData("externalData");

			// Reset "per frame" buffer for VS
			context->VSSetConstantBuffers(0, 1, vsPerFrameConstantBuffer.GetAddressOf());

			// Draw
			ge->GetMesh()->SetBuffersAndDraw(context);

			// Reset this material's PS
			mat->SetPS(prevPS);
		}

		// Reset depth state
		context->OMSetDepthStencilState(0, 0);
	}

	// now draw refractive objects
	renderTargets[0] = backBufferRTV.Get();
	context->OMSetRenderTargets(1, renderTargets, depthBufferDSV.Get());

	for (auto ge : refractiveEntities)
	{
		Material* material = ge->GetMaterial();
		SimplePixelShader* prevPS = material->GetPS();
		material->SetPS(refractionPS);

		// Overall material prep
		material->PrepareMaterial(ge->GetTransform(), camera);
		material->SetPerMaterialDataAndResources(true);

		// Set up the refraction specific data
		refractionPS->SetFloat2("screenSize", XMFLOAT2((float)windowWidth, (float)windowHeight));
		refractionPS->SetMatrix4x4("viewMatrix", camera->GetView());
		refractionPS->SetMatrix4x4("projMatrix", camera->GetProjection());
		refractionPS->SetInt("useRefractionSilhouette", useRefractionSilhouette);
		refractionPS->SetInt("refractionFromNormalMap", refractionFromNormalMap);
		refractionPS->SetFloat("indexOfRefraction", indexOfRefraction);
		refractionPS->SetFloat("refractionScale", refractionScale);
		refractionPS->CopyBufferData("perObject");

		// Set textures
		refractionPS->SetShaderResourceView("ScreenPixels", sceneColorsSRV);
		refractionPS->SetShaderResourceView("RefractionSilhouette", silhouetteSRV);
		refractionPS->SetShaderResourceView("EnvironmentMap", sky->GetSkySRV());

		// Reset "per frame" buffers
		context->VSSetConstantBuffers(0, 1, vsPerFrameConstantBuffer.GetAddressOf());
		context->PSSetConstantBuffers(0, 1, psPerFrameConstantBuffer.GetAddressOf());

		// Draw
		ge->GetMesh()->SetBuffersAndDraw(context);

		// Reset this material's PS
		material->SetPS(prevPS);
	}

	// Draw ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);

	// Due to the usage of a more sophisticated swap chain,
	// the render target must be re-bound after every call to Present()
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	ID3D11ShaderResourceView* nullSRVs[16] = {};
	context->PSSetShaderResources(0, 16, nullSRVs);
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetColorsRenderTargetSRV()
{
	return sceneColorsSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetNormalsRenderTargetSRV()
{
	return sceneNormalsSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetDepthsRenderTargetSRV()
{
	return sceneDepthsSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetSilhouetteRenderTargetSRV()
{
	return silhouetteSRV;
}

void Renderer::DrawPointLights(Camera* camera)
{
	// Turn on these shaders
	lightVS->SetShader();
	lightPS->SetShader();

	// Set up vertex shader
	lightVS->SetMatrix4x4("view", camera->GetView());
	lightVS->SetMatrix4x4("projection", camera->GetProjection());

	for (int i = 0; i < lightCount; i++)
	{
		Light light = lights[i];

		// Only drawing points, so skip others
		if (light.Type != LIGHT_TYPE_POINT)
			continue;

		// Calc quick scale based on range
		// (assuming range is between 5 - 10)
		float scale = light.Range / 10.0f;

		// Make the transform for this light
		XMMATRIX rotMat = XMMatrixIdentity();
		XMMATRIX scaleMat = XMMatrixScaling(scale, scale, scale);
		XMMATRIX transMat = XMMatrixTranslation(light.Position.x, light.Position.y, light.Position.z);
		XMMATRIX worldMat = scaleMat * rotMat * transMat;

		XMFLOAT4X4 world;
		XMFLOAT4X4 worldInvTrans;
		XMStoreFloat4x4(&world, worldMat);
		XMStoreFloat4x4(&worldInvTrans, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));

		// Set up the world matrix for this light
		lightVS->SetMatrix4x4("world", world);
		lightVS->SetMatrix4x4("worldInverseTranspose", worldInvTrans);

		// Set up the pixel shader data
		XMFLOAT3 finalColor = light.Color;
		finalColor.x *= light.Intensity;
		finalColor.y *= light.Intensity;
		finalColor.z *= light.Intensity;
		lightPS->SetFloat3("Color", finalColor);

		// Copy data
		lightVS->CopyAllBufferData();
		lightPS->CopyAllBufferData();

		// Draw
		lightMesh->SetBuffersAndDraw(context);
	}
}

void Renderer::CreateRenderTarget(unsigned int width, unsigned int height, Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> rtTexture;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // Need both!
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Might occasionally use other formats
	texDesc.MipLevels = 1; // Usually no mip chain needed for render targets
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1; // Can't be zero
	device->CreateTexture2D(&texDesc, 0, rtTexture.GetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;      // This points to a Texture2D
	rtvDesc.Texture2D.MipSlice = 0;                             // Which mip are we rendering into?
	rtvDesc.Format = texDesc.Format;                            // Same format as texture
	device->CreateRenderTargetView(rtTexture.Get(), &rtvDesc, rtv.GetAddressOf());

	device->CreateShaderResourceView(
		rtTexture.Get(),     // Texture resource itself
		0,                   // Null description = default SRV options
		srv.GetAddressOf()); // ComPtr<ID3D11ShaderResourceView>
}
