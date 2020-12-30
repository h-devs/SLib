#include <slib.h>

using namespace slib;

#define COUNT 256

int main(int argc, const char * argv[])
{ 
	Console::close();
	
	Plot plot;

	Complex c[COUNT];
	for (sl_uint32 i = 0; i < COUNT; i++) {
		sl_real x = (sl_real)i * 2 * SLIB_PI / COUNT - SLIB_PI;
		c[i].real = 0.5f * Math::cos(x * 5 + 0.1f) + Math::sin(x * 12 + 1.2f) + 0.8f * Math::sin(x * 2 + 2.0f);
		c[i].imag = 0;
	}

	plot.add(COUNT, [c](sl_uint32 i) {
		return c[i].real;
	}, PlotGraphParam(PlotGraphType::Line, Color::Red, 5));
	
	FFT fft(COUNT);
	fft.transform(c);

	plot.add(COUNT, [c](sl_uint32 i) {
		return c[i].real;
	}, PlotGraphParam(PlotGraphType::Line, Color::Green, 2));
	plot.add(COUNT, [c](sl_uint32 i) {
		return c[i].imag;
	}, PlotGraphParam(PlotGraphType::Line, Color::Blue, 2));

	fft.inverse(c);

	plot.add(COUNT, [c](sl_uint32 i) {
		return c[i].real;
	}, Color::White);

	auto window = plot.show(1000, 600);
	window->setQuitOnDestroy();
	
	UI::runApp();

	return 0;
}
