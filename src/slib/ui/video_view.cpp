/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/ui/video_view.h"

#include "slib/render/canvas.h"
#include "slib/render/opengl.h"
#include "slib/graphics/image.h"
#include "slib/graphics/util.h"
#include "slib/core/timer.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(VideoView, RenderView)

	VideoView::VideoView()
	{
		setSavingCanvasState(sl_false);

		setBackgroundColor(Color::Black, UIUpdateMode::Init);

		setRedrawMode(RedrawMode::WhenDirty);
		setDebugTextVisible(sl_false);

		m_flagRepeat = sl_true;
		m_rotation = RotationMode::Rotate0;
		m_flip = FlipMode::None;

		m_flagYUV = sl_false;
		m_flagAllowYUV = sl_true;
		m_rotationFrame = RotationMode::Rotate0;
		m_flipFrame = FlipMode::None;

		m_programRGB = new RenderProgram2D_PositionTexture;
		m_programYUV = new RenderProgram2D_PositionTextureYUV;
		m_programOES = new RenderProgram2D_PositionTextureOES;

		m_flipFrameApplied = FlipMode::None;
		m_rotationFrameApplied = RotationMode::Rotate0;
		m_flipApplied = FlipMode::None;
		m_rotationApplied = RotationMode::Rotate0;
		m_sizeLastFrame.x = 0;
		m_sizeLastFrame.y = 0;

		m_scaleMode = ScaleMode::Stretch;
		m_gravity = Alignment::MiddleCenter;

		m_flagControlsVisible = sl_false;
	}

	VideoView::~VideoView()
	{
		Ref<MediaPlayer> player = m_mediaPlayer;
		if (player.isNotNull()) {
			if (player->isAutoRelease()) {
				player->release();
			}
		}
		if (m_timerPlayVideo.isNotNull()) {
			m_timerPlayVideo->stopAndWait();
		}
	}

	void VideoView::init()
	{
		RenderView::init();

		m_renderVideoParam.onUpdateFrame = SLIB_FUNCTION_WEAKREF(this, updateCurrentFrame);
	}

	Ref<MediaPlayer> VideoView::getMediaPlayer()
	{
		return m_mediaPlayer;
	}

	void VideoView::setMediaPlayer(const Ref<MediaPlayer>& player)
	{
		m_mediaPlayer = player;
		if (player.isNotNull()) {
			setRedrawMode(RedrawMode::Continuously);
		} else {
			setRedrawMode(RedrawMode::WhenDirty);
		}
		_setupPlayVideoTimer();
	}

	void VideoView::openUrl(const String& url, const MediaPlayerFlags& _flags)
	{
		MediaPlayerFlags flags = _flags | MediaPlayerFlags::NotSelfAlive;
		if (m_flagRepeat) {
			flags |= MediaPlayerFlags::Repeat;
		}
		Ref<MediaPlayer> player = MediaPlayer::openUrl(url, flags);
		if (player.isNotNull()) {
			setMediaPlayer(player);
		}
	}

	void VideoView::openFile(const String& filePath, const MediaPlayerFlags& _flags)
	{
		MediaPlayerFlags flags = _flags | MediaPlayerFlags::NotSelfAlive;
		if (m_flagRepeat) {
			flags |= MediaPlayerFlags::Repeat;
		}
		Ref<MediaPlayer> player = MediaPlayer::openFile(filePath, flags);
		if (player.isNotNull()) {
			setMediaPlayer(player);
		}
	}

	void VideoView::openAsset(const String& fileName, const MediaPlayerFlags& _flags)
	{
		MediaPlayerFlags flags = _flags | MediaPlayerFlags::NotSelfAlive;
		if (m_flagRepeat) {
			flags |= MediaPlayerFlags::Repeat;
		}
		Ref<MediaPlayer> player = MediaPlayer::openAsset(fileName, flags);
		if (player.isNotNull()) {
			setMediaPlayer(player);
		}
	}

	void VideoView::setSource(const String& source, const MediaPlayerFlags& flags)
	{
		if (source.isNotEmpty()) {
			if (source.startsWith("asset://")) {
				openAsset(source.substring(8), flags);
			} else if (source.indexOf(':') >= 0) {
				openUrl(source, flags);
			} else {
				openFile(source, flags);
			}
		}
	}

	sl_bool VideoView::isRepeat()
	{
		return m_flagRepeat;
	}

	void VideoView::setRepeat(sl_bool flagRepeat)
	{
		m_flagRepeat = flagRepeat;
		Ref<MediaPlayer> player = m_mediaPlayer;
		if (player.isNotNull()) {
			player->setAutoRepeat(flagRepeat);
		}
	}

	RotationMode VideoView::getRotation()
	{
		return m_rotation;
	}

	void VideoView::setRotation(const RotationMode& rotation, UIUpdateMode mode)
	{
		m_rotation = rotation;
		invalidate(mode);
	}

	FlipMode VideoView::getFlip()
	{
		return m_flip;
	}

	void VideoView::setFlip(const FlipMode& flip, UIUpdateMode mode)
	{
		m_flip = flip;
		invalidate(mode);
	}

	ScaleMode VideoView::getScaleMode()
	{
		return m_scaleMode;
	}

	void VideoView::setScaleMode(ScaleMode scaleMode, UIUpdateMode mode)
	{
		m_scaleMode = scaleMode;
		invalidate(mode);
	}

	Alignment VideoView::getGravity()
	{
		return m_gravity;
	}

	void VideoView::setGravity(const Alignment& align, UIUpdateMode mode)
	{
		m_gravity = align;
		invalidate(mode);
	}

	sl_bool VideoView::isControlsVisible()
	{
		return m_flagControlsVisible;
	}

	void VideoView::setControlsVisible(sl_bool flag, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		m_flagControlsVisible = flag;
		if (flag) {
			if (m_sliderSeek.isNull()) {
				m_sliderSeek = new Slider;
				if (m_sliderSeek.isNotNull()) {
					m_sliderSeek->setWidthFilling(1.0f, UIUpdateMode::Init);
					m_sliderSeek->setHeightWeight(0.05f, UIUpdateMode::Init);
					m_sliderSeek->setAlignParentBottom(UIUpdateMode::Init);
					addChild(m_sliderSeek, mode);
					m_sliderSeek->setOnChange(SLIB_FUNCTION_WEAKREF(this, _onSeek));
				}
			}
			_updateControls(mode);
		}
		if (m_sliderSeek.isNotNull()) {
			m_sliderSeek->setVisible(flag, mode);
		}
	}

	void VideoView::updateCurrentFrame(VideoFrame& frame)
	{
		ColorSpace colorSpace = BitmapFormats::getColorSpace(frame.image.format);
		if (colorSpace != ColorSpace::RGB && colorSpace != ColorSpace::YUV) {
			return;
		}
		Ref<Texture> texture = m_textureFrame;
		if (texture.isNotNull()) {
			if (texture->getWidth() != frame.image.width || texture->getHeight() != frame.image.height) {
				texture.setNull();
			}
		}
		if (texture.isNull()) {
			texture = Texture::create(frame.image.width, frame.image.height);
		}
		if (texture.isNotNull()) {
			Ref<Image> image = Ref<Image>::from(texture->getSource());
			BitmapData bitmapData(image->getWidth(), image->getHeight(), image->getColors());

			sl_bool flagUseYUV = sl_false;
			if (m_flagAllowYUV) {
				flagUseYUV = BitmapFormats::getColorSpace(frame.image.format) == ColorSpace::YUV;
			}
			m_flagYUV = flagUseYUV;
			if (flagUseYUV) {
				bitmapData.format = BitmapFormat::YUVA;
			}
			m_rotationFrame = frame.rotation;
			m_flipFrame = frame.flip;
			bitmapData.copyPixelsFrom(frame.image);
			texture->update();
			image->update();
			m_textureFrame = texture;
			m_sizeLastFrame.x = frame.image.width;
			m_sizeLastFrame.y = frame.image.height;
		}
		requestRender();
	}

	SizeI VideoView::getLastFrameSize()
	{
		return m_sizeLastFrame;
	}

	sl_bool VideoView::convertCoordinateToTexture(Point& pt)
	{
		sl_int32 sw = m_sizeLastFrame.x;
		sl_int32 sh = m_sizeLastFrame.y;
		if (sw <= 0 || sh <= 0) {
			return sl_false;
		}
		if (m_rotationFrameApplied == RotationMode::Rotate90 || m_rotationFrameApplied == RotationMode::Rotate270) {
			Swap(sw, sh);
		}
		if (m_rotationApplied == RotationMode::Rotate90 || m_rotationApplied == RotationMode::Rotate270) {
			Swap(sw, sh);
		}
		Rectangle rectDraw;
		GraphicsUtil::calculateAlignRectangle(rectDraw, getBoundsInnerPadding(), (sl_real)sw, (sl_real)sh, m_scaleMode, m_gravity);
		sl_real w = rectDraw.getWidth();
		if (w < SLIB_EPSILON) {
			return sl_false;
		}
		sl_real h = rectDraw.getHeight();
		if (h < SLIB_EPSILON) {
			return sl_false;
		}
		sl_real x = (pt.x - rectDraw.left) / w;
		sl_real y = (pt.y - rectDraw.top) / h;
		RotatePoint(x, y, (sl_real)1, (sl_real)1, -(m_rotationApplied));
		FlipPoint(x, y, (sl_real)1, (sl_real)1, m_flipApplied);
		RotatePoint(x, y, (sl_real)1, (sl_real)1, -(m_rotationFrameApplied));
		FlipPoint(x, y, (sl_real)1, (sl_real)1, m_flipFrameApplied);
		pt.x = x;
		pt.y = y;
		return sl_true;
	}

	void VideoView::dispatchFrame(RenderEngine* engine)
	{
		if (engine->isShaderAvailable()) {
			m_flagAllowYUV = sl_true;
			RenderView::dispatchFrame(engine);
		} else {
			m_flagAllowYUV = sl_false;
			disableRendering();
			_setupPlayVideoTimer();
		}
	}

	void VideoView::onDraw(Canvas* _canvas)
	{
		_updateControls(UIUpdateMode::None);
		RenderCanvas* canvas = CastInstance<RenderCanvas>(_canvas);
		if (canvas) {
			_renderFrame(canvas);
		} else {
			m_flagAllowYUV = sl_false;
			_drawFrame(_canvas);
		}
	}

	void VideoView::onAttach()
	{
		_setupPlayVideoTimer();
	}

	void VideoView::_renderFrame(RenderCanvas* canvas)
	{
		Ref<RenderEngine> engine = canvas->getEngine();
		if (engine.isNull()) {
			return;
		}
		UIRect rectBoundsi = getBoundsInnerPadding();
		if (!(rectBoundsi.isValidSize())) {
			return;
		}
		Rectangle rectBounds = rectBoundsi;
		Ref<MediaPlayer> mediaPlayer = m_mediaPlayer;
		if (mediaPlayer.isNotNull()) {
			m_renderVideoParam.glEngine = CastRef<GLRenderEngine>(engine);
			mediaPlayer->renderVideo(m_renderVideoParam);
		}
		Ref<Texture> texture;
		Matrix3 textureMatrix;
		Ref<RenderProgram2D_PositionTexture> program;
		if (m_renderVideoParam.glTextureOES.isNotNull()) {
			texture = m_renderVideoParam.glTextureOES;
			textureMatrix = m_renderVideoParam.glTextureTransformOES;
			program = m_programOES;
			m_sizeLastFrame.x = texture->getWidth();
			m_sizeLastFrame.y = texture->getHeight();
		} else {
			texture = m_textureFrame;
			textureMatrix = Matrix3::identity();
			if (m_flagYUV) {
				program = m_programYUV;
			} else {
				program = m_programRGB;
			}
		}
		Ref<VertexBuffer> vb = _applyFrameRotationAndFlip(m_flipFrame, m_rotationFrame, m_flip, m_rotation);
		if (vb.isNotNull() && texture.isNotNull() && program.isNotNull()) {
			sl_real sw = (sl_real)(texture->getWidth());
			sl_real sh = (sl_real)(texture->getHeight());
			if (m_rotationFrameApplied == RotationMode::Rotate90 || m_rotationFrameApplied == RotationMode::Rotate270) {
				Swap(sw, sh);
			}
			if (m_rotationApplied == RotationMode::Rotate90 || m_rotationApplied == RotationMode::Rotate270) {
				Swap(sw, sh);
			}
			Rectangle rectDraw;
			if (!(GraphicsUtil::calculateAlignRectangle(rectDraw, rectBounds, sw, sh, m_scaleMode, m_gravity))) {
				return;
			}
			Matrix3 mat = canvas->getTransformMatrixForRectangle(rectDraw);
			RenderProgramScope<RenderProgramState2D_PositionTexture> scope;
			if (scope.begin(engine.get(), program)) {
				scope->setTransform(mat);
				scope->setTexture(texture);
				scope->setTextureTransform(textureMatrix);
				scope->setColor(Color4F(1, 1, 1, canvas->getAlpha()));
				engine->drawPrimitive(4, vb, PrimitiveType::TriangleStrip);
			}
		}
	}

	void VideoView::_drawFrame(Canvas* canvas)
	{
		UIRect rectBoundsi = getBoundsInnerPadding();
		if (!(rectBoundsi.isValidSize())) {
			return;
		}
		Rectangle rectBounds = rectBoundsi;
		Ref<Texture> texture = m_textureFrame;
		if (texture.isNull()) {
			return;
		}
		sl_uint32 tw = texture->getWidth();
		sl_uint32 th = texture->getHeight();
		if (!tw || !th) {
			return;
		}
		Ref<Bitmap> bitmap = texture->getSource();
		if (bitmap.isNull()) {
			return;
		}
		sl_real sw = (sl_real)(tw);
		sl_real sh = (sl_real)(th);
		Rectangle rectDraw;
		if (!(GraphicsUtil::calculateAlignRectangle(rectDraw, rectBounds, sw, sh, m_scaleMode, m_gravity))) {
			return;
		}
		canvas->draw(rectDraw, bitmap);
	}

	Ref<VertexBuffer> VideoView::_applyFrameRotationAndFlip(FlipMode frameFlip, RotationMode frameRotation, FlipMode userFlip, RotationMode userRotation)
	{
		if (m_vbFrame.isNotNull() && m_flipFrameApplied == frameFlip && m_rotationFrameApplied == frameRotation && m_flipApplied == userFlip && m_rotationApplied == userRotation) {
			return m_vbFrame;
		}
		RenderVertex2D_PositionTexture v[] = {
			{ { 0, 0 }, { 0, 0 } }
			, { { 1, 0 }, { 1, 0 } }
			, { { 0, 1 }, { 0, 1 } }
			, { { 1, 1 }, { 1, 1 } }
		};
		Vector2 t;
		switch (frameFlip) {
			case FlipMode::Horizontal:
				Swap(v[0].texCoord, v[1].texCoord);
				Swap(v[2].texCoord, v[3].texCoord);
				break;
			case FlipMode::Vertical:
				Swap(v[0].texCoord, v[2].texCoord);
				Swap(v[1].texCoord, v[3].texCoord);
				break;
			case FlipMode::Both:
				Swap(v[0].texCoord, v[3].texCoord);
				Swap(v[1].texCoord, v[2].texCoord);
				break;
			default:
				break;
		}
		switch (frameRotation) {
			case RotationMode::Rotate90:
				t = v[0].texCoord;
				v[0].texCoord = v[2].texCoord;
				v[2].texCoord = v[3].texCoord;
				v[3].texCoord = v[1].texCoord;
				v[1].texCoord = t;
				break;
			case RotationMode::Rotate180:
				Swap(v[0].texCoord, v[3].texCoord);
				Swap(v[1].texCoord, v[2].texCoord);
				break;
			case RotationMode::Rotate270:
				t = v[0].texCoord;
				v[0].texCoord = v[1].texCoord;
				v[1].texCoord = v[3].texCoord;
				v[3].texCoord = v[2].texCoord;
				v[2].texCoord = t;
				break;
			default:
				break;
		}
		switch (userFlip) {
			case FlipMode::Horizontal:
				Swap(v[0].texCoord, v[1].texCoord);
				Swap(v[2].texCoord, v[3].texCoord);
				break;
			case FlipMode::Vertical:
				Swap(v[0].texCoord, v[2].texCoord);
				Swap(v[1].texCoord, v[3].texCoord);
				break;
			case FlipMode::Both:
				Swap(v[0].texCoord, v[3].texCoord);
				Swap(v[1].texCoord, v[2].texCoord);
				break;
			default:
				break;
		}
		switch (userRotation) {
			case RotationMode::Rotate90:
				t = v[0].texCoord;
				v[0].texCoord = v[2].texCoord;
				v[2].texCoord = v[3].texCoord;
				v[3].texCoord = v[1].texCoord;
				v[1].texCoord = t;
				break;
			case RotationMode::Rotate180:
				Swap(v[0].texCoord, v[3].texCoord);
				Swap(v[1].texCoord, v[2].texCoord);
				break;
			case RotationMode::Rotate270:
				t = v[0].texCoord;
				v[0].texCoord = v[1].texCoord;
				v[1].texCoord = v[3].texCoord;
				v[3].texCoord = v[2].texCoord;
				v[2].texCoord = t;
				break;
			default:
				break;
		}
		Ref<VertexBuffer> vb = VertexBuffer::create(v, sizeof(v));
		m_vbFrame = vb;
		m_flipFrameApplied = frameFlip;
		m_rotationFrameApplied = frameRotation;
		m_flipApplied = userFlip;
		m_rotationApplied = userRotation;
		return vb;
	}

	void VideoView::_updateControls(UIUpdateMode mode)
	{
		Ref<MediaPlayer> player = m_mediaPlayer;
		if (player.isNull()) {
			return;
		}
		if (m_sliderSeek.isNotNull()) {
			double duration = player->getDuration();
			if (duration > 0) {
				m_sliderSeek->setMaximumValue((float)duration, UIUpdateMode::None);
				m_sliderSeek->setValue((float)(player->getCurrentTime()), mode);
			} else {
				m_sliderSeek->setMaximumValue(1, UIUpdateMode::None);
				m_sliderSeek->setValue(0, mode);
			}
		}
	}

	void VideoView::_setupPlayVideoTimer()
	{
		ObjectLocker lock(this);
		if (!(isInstance())) {
			return;
		}
		if (m_mediaPlayer.isNotNull() && !(isRenderEnabled())) {
			if (m_timerPlayVideo.isNull()) {
				m_timerPlayVideo = Timer::start(SLIB_FUNCTION_WEAKREF(this, _onTimerPlayVideo), 30);
			}
		} else {
			m_timerPlayVideo.setNull();
		}
	}

	void VideoView::_onTimerPlayVideo(Timer* timer)
	{
		Ref<MediaPlayer> mediaPlayer = m_mediaPlayer;
		if (mediaPlayer.isNotNull()) {
			mediaPlayer->renderVideo(m_renderVideoParam);
		}
	}

	void VideoView::_onSeek(Slider* slider, float value)
	{
		Ref<MediaPlayer> player = m_mediaPlayer;
		if (player.isNotNull()) {
			player->seekTo(value);
		}
	}

}
