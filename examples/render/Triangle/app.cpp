#include "app.h"

TriangleApp::TriangleApp()
{
}

void TriangleApp::onStart()
{
	RenderVertex2D_Position vertices[] = { {Vector2(0, 0)}, {Vector2(0, 1)}, { Vector2(1, 0)} };
	auto vb = VertexBuffer::create(vertices, sizeof(vertices));
	auto program = New<RenderProgram2D_Position>();

	auto view = New<RenderView>();
	view->setWidthFilling(1, UIUpdateMode::Init);
	view->setHeightFilling(1, UIUpdateMode::Init);
	view->setRedrawMode(RedrawMode::Continuously);

	view->setOnFrame([vb, program](RenderView*, RenderEngine* engine) {
		engine->clearColor(Color::Blue);
		RenderProgramScope<RenderProgramState2D_Position> scope;
		if (scope.begin(engine, program)) {
			scope->setTransform(Transform2::getRotationMatrix((sl_real)((Time::now().getMillisecondCount() % 1000) * SLIB_PI_DUAL / 1000)));
			scope->setColor(Color(Color::Red));
			engine->drawPrimitive(3, vb);
		}
	});

	auto window = New<Window>();
	window->setTitle("Triangle");
	window->setFrame(100, 100, 400, 300);
	window->setResizable();
	window->addView(view, UIUpdateMode::Init);
	window->show();
	setMainWindow(window);
}
