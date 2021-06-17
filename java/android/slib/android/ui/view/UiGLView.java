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

package slib.android.ui.view;

import java.util.Vector;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import slib.android.Logger;
import slib.android.SlibActivity;
import slib.android.ui.Util;

import android.app.Activity;
import android.content.Context;
import android.graphics.Rect;
import android.opengl.GLSurfaceView;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

public class UiGLView extends GLSurfaceView implements IView, GLSurfaceView.Renderer {

	private long mInstance = 0;
	public long getInstance() { return mInstance; }
	public void setInstance(long instance) { this.mInstance = instance; }
	private int mLeft, mTop, mRight, mBottom;
	public Rect getUIFrame() { return new Rect(mLeft, mTop, mRight, mBottom); }
	public void setUIFrame(int left, int top, int right, int bottom) { mLeft = left; mTop = top; mRight = right; mBottom = bottom; }
	private boolean mStopPropagation = false;
	public boolean isStopPropagation() { return mStopPropagation; }
	public void setStopPropagation(boolean flag) { mStopPropagation = flag; }
	public boolean dispatchSuperTouchEvent(MotionEvent ev) { return super.dispatchTouchEvent(ev); }

	UiGestureDetector gestureDetector;

	private static final Object sync = new Object();
	private static Vector<UiGLView> glViewList = new Vector<UiGLView>();
	
	public static UiGLView _create(Context context) {
		try {
			initialize();
			return new UiGLView(context);
		} catch (Exception e) {
			Logger.exception(e);
		}
		return null;
	}
	
	public static boolean _setRenderMode(final View view, final int mode) {
		try {
			if (view instanceof GLSurfaceView) {
				if (mode == 0) {
					((GLSurfaceView) view).setRenderMode(UiGLView.RENDERMODE_CONTINUOUSLY);
				} else {
					((GLSurfaceView) view).setRenderMode(UiGLView.RENDERMODE_WHEN_DIRTY);
				}
				return true;
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
		return false;
	}

	public static void _requestRender(final View view) {
		try {
			if (view instanceof UiGLView) {
				((UiGLView) view).requestRender();
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}
	

	private static native void nativeOnCreate(long instance);
	public static void onEventCreate(IView view) {
		long instance = view.getInstance();
		if (instance != 0) {
			nativeOnCreate(instance);
		}
	}
	
	private static native void nativeOnFrame(long instance, int width, int height);
	public static void onEventFrame(IView view, int width, int height) {
		long instance = view.getInstance();
		if (instance != 0) {
			nativeOnFrame(instance, width, height);
		}
	}
	
	public UiGLView(Context context) {
		super(context);

		setEGLContextClientVersion(2);
		setEGLConfigChooser(new ConfigChooser(true, false));
		
		setRenderer(this);
		setRenderMode(RENDERMODE_WHEN_DIRTY);

		setClickable(true);
	}

	@Override
	public boolean onKeyDown(int keycode, KeyEvent event) {
		if (!(UiView.onEventKey(this, true, keycode, event))) {
			return super.onKeyDown(keycode, event);
		}
		return true;
	}

	@Override
	public boolean onKeyUp(int keycode, KeyEvent event) {
		if (!(UiView.onEventKey(this, false, keycode, event))) {
			return super.onKeyUp(keycode, event);
		}
		return true;
	}
	
	@Override
	public boolean dispatchTouchEvent(MotionEvent event) {
		if (gestureDetector != null) {
			gestureDetector.onTouchEvent(event);
			UiView.dispatchEventTouch(this, event);
			return true;
		} else {
			return UiView.dispatchEventTouch(this, event);
		}
	}

	@Override
	protected void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
		super.onFocusChanged(focused, direction, previouslyFocusedRect);
		if (focused) {
			UiView.onEventSetFocus(this);
		}
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		onEventCreate(this);
		synchronized (sync) {
			if (!(glViewList.contains(this))) {
				glViewList.add(this);
			}
		}
	}

	@Override
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		widthViewport = width;
		heightViewport = height;
	}

	int widthViewport = 0;
	int heightViewport = 0;

	@Override
	public void onDrawFrame(GL10 gl) {
		onEventFrame(this, widthViewport, heightViewport);
	}
	
	public static void removeView(final View view) {
		if (!(Util.isUiThread())) {
			view.post(new Runnable() {
				public void run() {
					removeView(view);
				}
			});
			return;
		}
		if (view instanceof UiGLView) {
			synchronized (sync) {
				glViewList.remove((UiGLView)view);
			}
		}
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		setMeasuredDimension(UiView.resolveMeasure(mRight-mLeft, widthMeasureSpec), UiView.resolveMeasure(mBottom-mTop, heightMeasureSpec));
	}


	private static boolean mFlagInitialized = false;

	private static synchronized void initialize() {
		if (mFlagInitialized) {
			return;
		}
		mFlagInitialized = true;
		SlibActivity.addResumeActivityListener(new SlibActivity.ResumeActivityListener() {
			@Override
			public void onResumeActivity(Activity activity) {
				synchronized (sync) {
					try {
						for (UiGLView view : glViewList) {
							try {
								view.onResume();
							} catch (Exception e) {
								Logger.exception(e);
							}
						}
					} catch (Exception e) {
						Logger.exception(e);
					}
				}
			}
		});
		SlibActivity.addPauseActivityListener(new SlibActivity.PauseActivityListener() {
			@Override
			public void onPauseActivity(Activity activity) {
				synchronized (sync) {
					try {
						for (UiGLView view : glViewList) {
							try {
								view.onPause();
							} catch (Exception e) {
								Logger.exception(e);
							}
						}
					} catch (Exception e) {
						Logger.exception(e);
					}
				}
			}
		});
	}


	class ConfigChooser implements GLSurfaceView.EGLConfigChooser {

		boolean mIsRGBA;
		boolean mIsMultisample;

		ConfigChooser(boolean flagRGBA, boolean flagMultisample) {
			mIsRGBA = flagRGBA;
			mIsMultisample = flagMultisample;
		}

		@Override
		public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {

			EGLConfig[] configs = new EGLConfig[1];
			int[] configsCount = new int[1];
			int[] configSpec;

			if (mIsMultisample) {

				if (mIsRGBA) {
					configSpec = new int[] {
							EGL10.EGL_RED_SIZE, 8,
							EGL10.EGL_GREEN_SIZE, 8,
							EGL10.EGL_BLUE_SIZE, 8,
							EGL10.EGL_ALPHA_SIZE, 8,
							EGL10.EGL_DEPTH_SIZE, 24,
							EGL10.EGL_STENCIL_SIZE,	8,
							// Requires that setEGLContextClientVersion(2) is called on the view.
							EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
							EGL10.EGL_SAMPLE_BUFFERS, 1 /* true */,
							EGL10.EGL_SAMPLES, 2,
							EGL10.EGL_NONE };
					if (egl.eglChooseConfig(display, configSpec, configs, 1, configsCount)) {
						if (configsCount[0] == 1) {
							return configs[0];
						}
					}
				}

				configSpec = new int[] {
						EGL10.EGL_RED_SIZE, 5,
						EGL10.EGL_GREEN_SIZE, 6,
						EGL10.EGL_BLUE_SIZE, 5,
						EGL10.EGL_DEPTH_SIZE, 16,
						EGL10.EGL_STENCIL_SIZE, 8,
						// Requires that setEGLContextClientVersion(2) is called on the view.
						EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
						EGL10.EGL_SAMPLE_BUFFERS, 1 /* true */,
						EGL10.EGL_SAMPLES, 2,
						EGL10.EGL_NONE };
				if (egl.eglChooseConfig(display, configSpec, configs, 1, configsCount)) {
					if (configsCount[0] == 1) {
						return configs[0];
					}
				}

				// No normal multisampling config was found. Try to create a
				// converage multisampling configuration, for the nVidia Tegra2.
				// See the EGL_NV_coverage_sample documentation.
				final int EGL_COVERAGE_BUFFERS_NV = 0x30E0;
				final int EGL_COVERAGE_SAMPLES_NV = 0x30E1;

				if (mIsRGBA) {

					configSpec = new int[] {
							EGL10.EGL_RED_SIZE, 8,
							EGL10.EGL_GREEN_SIZE, 8,
							EGL10.EGL_BLUE_SIZE, 8,
							EGL10.EGL_ALPHA_SIZE, 8,
							EGL10.EGL_DEPTH_SIZE, 24,
							EGL10.EGL_STENCIL_SIZE, 8,
							EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
							EGL_COVERAGE_BUFFERS_NV, 1 /* true */,
							EGL_COVERAGE_SAMPLES_NV, 2, // always 5 in practice on tegra 2
							EGL10.EGL_NONE };
					if (egl.eglChooseConfig(display, configSpec, configs, 1, configsCount)) {
						if (configsCount[0] == 1) {
							return configs[0];
						}
					}
				}

				configSpec = new int[] {
						EGL10.EGL_RED_SIZE, 5,
						EGL10.EGL_GREEN_SIZE, 6,
						EGL10.EGL_BLUE_SIZE, 5,
						EGL10.EGL_DEPTH_SIZE, 16,
						EGL10.EGL_STENCIL_SIZE, 8,
						EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
						EGL_COVERAGE_BUFFERS_NV, 1 /* true */,
						EGL_COVERAGE_SAMPLES_NV, 2, // always 5 in practice on tegra 2
						EGL10.EGL_NONE };
				if (egl.eglChooseConfig(display, configSpec, configs, 1, configsCount)) {
					if (configsCount[0] == 1) {
						return configs[0];
					}
				}

			}

			// Give up, try without multisampling.
			if (mIsRGBA) {
				configSpec = new int[] {
						EGL10.EGL_RED_SIZE, 8,
						EGL10.EGL_GREEN_SIZE, 8,
						EGL10.EGL_BLUE_SIZE, 8,
						EGL10.EGL_ALPHA_SIZE, 8,
						EGL10.EGL_DEPTH_SIZE, 24,
						EGL10.EGL_STENCIL_SIZE, 8,
						// Requires that setEGLContextClientVersion(2) is called on the view.
						EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
						EGL10.EGL_NONE };
				if (egl.eglChooseConfig(display, configSpec, configs, 1, configsCount)) {
					if (configsCount[0] == 1) {
						return configs[0];
					}
				}
			}
			configSpec = new int[] {
					EGL10.EGL_RED_SIZE, 5,
					EGL10.EGL_GREEN_SIZE, 6,
					EGL10.EGL_BLUE_SIZE, 5,
					EGL10.EGL_DEPTH_SIZE, 16,
					EGL10.EGL_STENCIL_SIZE, 8,
					// Requires that setEGLContextClientVersion(2) is called on the view.
					EGL10.EGL_RENDERABLE_TYPE, 4 /* EGL_OPENGL_ES2_BIT */,
					EGL10.EGL_NONE };
			if (egl.eglChooseConfig(display, configSpec, configs, 1, configsCount)) {
				if (configsCount[0] == 1) {
					return configs[0];
				}
			}
			throw new IllegalArgumentException("eglChooseConfig failed");
		}

	}

}