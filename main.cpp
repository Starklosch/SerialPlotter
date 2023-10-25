// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

//#include <kfr/base.hpp>
//#include <kfr/dft.hpp>
//#include <kfr/dsp.hpp>
//#include <kfr/io.hpp>
#include <fftw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.

#include <implot.h>
#include <thread>
#include <queue>
#include <fstream>
#include <chrono>
#include "Serial.h"
#include <imgui_internal.h>
//#include <math.h>

double magnitude(fftw_complex complex) {
	return sqrt(complex[0] * complex[0] + complex[1] * complex[1]);
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

float width = 1280, height = 720;

void window_resize(GLFWwindow* window, int w, int h) {
	width = w;
	height = h;
	//ImGui::SetWindowSize("Hello, world!", { (float)width, (float)height });
}

int MetricFormatter(double value, char* buff, int size, void* data) {
	const char* unit = (const char*)data;
	static double v[] = { 1000000000,1000000,1000,1,0.001,0.000001,0.000000001 };
	static const char* p[] = { "G","M","k","","m","u","n" };
	if (value == 0) {
		return snprintf(buff, size, "0 %s", unit);
	}
	for (int i = 0; i < 7; ++i) {
		if (fabs(value) >= v[i]) {
			return snprintf(buff, size, "%g %s%s", value / v[i], p[i], unit);
		}
	}
	return snprintf(buff, size, "%g %s%s", value / v[6], p[6], unit);
}

using namespace std::chrono_literals;
using namespace std::string_literals;

#include "Buffers.h"

std::vector<std::string> EnumerateComPorts() {
	std::vector<std::string> com_ports;

	HKEY key;
	LSTATUS result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &key);
	if (result != ERROR_SUCCESS)
		return com_ports;

	for (DWORD i = 0; result != ERROR_NO_MORE_ITEMS; i++)
	{
		char name[64];
		uint8_t data[64];
		DWORD nameChars = 64, dataSize = 64;
		DWORD type;
		result = RegEnumValueA(key, i, name, &nameChars, 0, &type, data, &dataSize);
		//if (error == ERROR_SUCCESS) {
		//	std::cout << name << " (" << typeToString(type) << "):\n";
		//	printTypeValue(type, data, dataSize);
		//	std::cout << '\n';
		//}

		if (result == ERROR_SUCCESS)
			com_ports.emplace_back((char*)data);
	}
	RegCloseKey(key);

	std::sort(com_ports.begin(), com_ports.end());
	return com_ports;
}

bool Button(const char *label, bool disabled = false) {
	if (disabled) {
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	bool result = ImGui::Button(label);
	if (disabled) {
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
	}
	return result;
}


void reception_test() {
	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
	using duration = std::chrono::duration<double>;
	time_point start = clock::now(), now;
	double dur = 0;
	int read = 0;

	Serial ser;
	ser.open("COM5", 38400);
	uint8_t buff[1024];
	while (dur <= 1) {
		now = clock::now();
		dur = duration(now - start).count();
		read += ser.read(buff, 1024);
	}
	std::cout << std::format("{} bytes read in {} seconds.\n", read, dur);
}

void monitor_test() {
	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
	using duration = std::chrono::duration<double>;

	std::vector<uint8_t> buff(4 * 1024);
	Monitor monitor(16 * 1024, 2 * 1024, 1024);
	int read = 0;

	monitor.start("COM5", 38400);
	
	std::this_thread::sleep_for(1s);

	int available = monitor.available();
	
	time_point start = clock::now(), now;
	read = monitor.read(buff.data(), available);
	now = clock::now();

	std::cout << std::format("{} bytes available\n", available);
	std::cout << std::format("{} bytes read in {} seconds.\n", read, duration(now - start).count());
	std::cout << std::format("{} bytes still available\n", monitor.available());
}

// Main code
int main(int, char**)
{
	monitor_test(); return 0;

	const int N = 1024;
	fftw_complex* out; /* Output */
	fftw_plan p; /* Plan */

	/*
	 * Size of output is (N / 2 + 1) because the other remaining items are
	 * redundant, in the sense that they are complex conjugate of already
	 * computed ones.
	 *
	 * CASE SIZE 6 (even):
	 * [real 0][complex 1][complex 2][real 3][conjugate of 2][conjugate of 1]
	 *
	 * CASE SIZE 5 (odd):
	 * [real 0][complex 1][complex 2][conjugate of 2][conjugate of 1]
	 *
	 * In both cases the items following the first N/2+1 are redundant.
	 */
	const int out_N = N / 2 + 1;
	out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * out_N);

	std::vector<double> in(N);
	std::vector<double> in_x(N);
	std::vector<double> fourier(out_N);
	std::vector<double> fourier_x(out_N);
	p = fftw_plan_dft_r2c_1d(N, in.data(), out, FFTW_ESTIMATE);

	for (size_t i = 0; i < out_N; i++)
		fourier_x[i] = i;

	//for (size_t i = 0; i < N; i++)
	//	in_x[i] = i / (float)N;

	//double pi = 3.14159265359;
	//for (int i = 0; i < N; i++)
	//	in[i] = 3 * sin(2 * pi * i / (double)N) + sin(2 * pi * 4 * i / (double)N) + 0.5 * sin(2 * pi * 7 * i / (double)N);

	//fftw_execute(p);

	//for (int i = 0; i < out_N; i++)
	//	fourier[i] = magnitude(out[i]);


	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Procesamiento Digital de Señales", nullptr, nullptr);
	if (window == nullptr)
		return 1;

	glfwSetWindowSizeCallback(window, window_resize);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	// - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	// Our state
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	const float e = 2.71828182846f;

	int selected_baud = 38400;

	int max_time = 60;
	int read_frequency = 30;
	float factor = 1;//pow(e, -(baud - 9600) / 200000.f);

	float show_time = 5 * factor;
	float offset = 0;// -100e-3;

	float max_time_visible = 10 * factor;
	float max_time_scale = 10.0f;
	float min_time_scale = 5e-4;

	//std::cout << std::format("Factor {} Max {} Min {} Show_time {}\n", factor, max_time_scale, min_time_scale, show_time);

	int speed, each, max_size, size = 0, view_size = 0;

	uint8_t* read_buffer = nullptr;
	//Buffer<uint8_t>* write_buffer = nullptr;

	ScrollBuffer<double>* scrollX = nullptr, * scrollY = nullptr;
	float* xs = 0, * ys = 0;

	const auto destroy_buffers = [&] {
		delete[] read_buffer;
		delete scrollX;
		delete scrollY;
		delete[] xs;
		delete[] ys;
	};

	const auto init_buffers = [&] {
		speed = selected_baud / 10;
		each = speed / read_frequency;
		max_size = speed * max_time;
		size = 0;
		view_size = max_time_visible * speed;
		std::cout << std::format("Speed: {}\n", speed);

		destroy_buffers();
		read_buffer = new uint8_t[each];
		scrollX = new ScrollBuffer<double>(max_size, view_size);
		scrollY = new ScrollBuffer<double>(max_size, view_size);
		xs = new float[view_size];
		ys = new float[view_size];
	};

	init_buffers();

	Monitor monitor(16 * 1024, 2 * 1024, 1024);
	Serial serial;
	bool run_receive = true, run_transmission = true, serial_started = false;
	bool show = true;

	std::string selected_port;
	std::string selected_baud_str = std::to_string(selected_baud);

	float start_time = 0;

	float last_time = 0;
	float elapsed_time = offset;
	bool avanzar = true;
	int draw_size = 0;
	int maximo = 255, minimo = 0;
	float frecuencia_muestreo = 960;

	//std::ofstream log("C:/Users/usuario/Desktop/log.txt");

	auto receive = [&] {
		// 60 - 130
		// 10 - 60
		// (x - 10) / (60 - 10) * 12 - 6
		float factor = 12 / 255.f;
		using clock = std::chrono::high_resolution_clock;
		using time_point = clock::time_point;
		using duration = std::chrono::duration<double>;
		time_point start = clock::now(), last_time;
		int times = 0;
		bool show = true;
		int last_read = 0;
		while (run_receive) {
			if (!serial_started) {
				times = 0;
				start = clock::now();
				last_time = start;
				last_read = 0;


				continue;
			}

			//std::cout << std::format("{} bytes available\n", serial.available());
			int read = serial.read(read_buffer, each);
			//std::cout << std::format("{} bytes read\n", read);
			//std::cout << std::format("{} read\n", read);
			//write_buffer->write(read_buffer, read);

			//float now = ImGui::GetTime();
			//float elapsed = now - last_time;
			auto now = clock::now();
			auto relative_time = duration(now - start);
			float elapsed = duration(now - last_time).count();
			float last_relative_time = duration(last_time - start).count();
			//times++;

			//if (duration.count() > 1 && show) {
			//	std::cout << std::format("{} times in {} s\n", times, duration.count());
			//	show = false;
			//}


			for (size_t i = 0; i < read; i++)
			{
				//ysbuf[size + i] = buffer[i] * factor - 6;
				scrollY->push((read_buffer[i] - minimo) / (float)(maximo - minimo) * 12 - 6);
				//buffer[i] = buffer[i] * factor - 6;
			}
			for (size_t i = 0; i < last_read; i++)
			{
				scrollX->push(last_relative_time + i * elapsed / last_read);
				//std::cout << last_relative_time + i * elapsed / read << ", ";
			}
			//if (speed <= scrollX->size() && show) {
			//	show = false;
			//}
			last_time = now;
			last_read = read;
			//ysbuf.write(buffer, read);
			
			size = scrollX->count();
		}
	};

	//const float pi = 3.14159265359f;
	//float signal_buffer[512];
	//for (int i = 0; i < 512; i++)
	//{
	//	signal_buffer[i] = sin(2 * pi * i);
	//}
	


	//auto transmit = [&] {
	//	// 60 - 130
	//	// 10 - 60
	//	// (x - 10) / (60 - 10) * 12 - 6
	//	float factor = 12 / 255.f;
	//	uint32_t it = 0;
	//	while (run_transmission) {
	//		if (!serial_started)
	//			continue;

	//		int size = write_buffer->size();
	//		if (size > 0)
	//			serial.write(write_buffer->data(), size);
	//	}
	//};

	std::thread fetch_thread(receive);
	float left_limit = 0, right_limit = max_time_visible;

	auto toggle_serial = [&] {
		if (!serial_started) {
			if (selected_port.empty())
				return;

			init_buffers();
			serial.open(selected_port.c_str(), selected_baud);
			start_time = ImGui::GetTime();
			scrollX->clear();
			scrollY->clear();
		}
		else
			serial.close();
		serial_started = !serial_started;
	};


	// Main loop
#ifdef __EMSCRIPTEN__
	// For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
	// You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
	io.IniFilename = nullptr;
	EMSCRIPTEN_MAINLOOP_BEGIN
#else
	while (!glfwWindowShouldClose(window))
#endif
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			auto puertos = EnumerateComPorts();

			static float f = 0.0f;
			static int counter = 0;
			if (serial_started && avanzar) {
				float now = ImGui::GetTime();
				elapsed_time = now - start_time;
				draw_size = size;
				int count = scrollX->count();
				std::copy(scrollX->data(), scrollX->data() + count, xs);
				std::copy(scrollY->data(), scrollY->data() + count, ys);

				left_limit = scrollX->front();
				right_limit = left_limit + show_time;

				std::transform(ys, ys + N, in.data(), [](double d) { return d; });
				fftw_execute(p);
				std::transform(out, out + out_N, fourier.data(), magnitude);

				//if (count == 960) {
				//	for (int i = 0; i < N; i++)
				//		log << in[i] << ", ";
				//	log << '\n';
				//	for (int i = 0; i < out_N; i++)
				//		log << fourier[i] << ", ";
				//	log << '\n';
				//}
			}

			ImGui::SetNextWindowPos({ 0, 0 });
			ImGui::SetNextWindowSize({ width, height });
			ImGui::Begin("Hello, world!", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);                          // Create a window called "Hello, world!" and append into it.

			if (puertos.empty()) {
				ImGui::Text("Conecta un dispositivo");
			}
			else if (ImGui::BeginCombo("Puerto", selected_port.empty() ? "Seleccionar puerto" : selected_port.c_str())) {
				for (const auto& puerto : puertos)
					if (ImGui::Selectable(puerto.c_str()))
						selected_port = puerto.c_str();
				ImGui::EndCombo();
			}

			const int bauds[] = { 9600, 14400, 19200, 38400, 100000, 115200, 1000000, 2000000 };
			if (ImGui::BeginCombo("Velocidad", selected_baud_str.c_str())) {
				for (const auto& baud : bauds) {
					auto str = std::to_string(baud);
					if (ImGui::Selectable(str.c_str())) {
						selected_baud = baud;
						selected_baud_str = std::move(str);
					}
				}
				ImGui::EndCombo();
			}

			if (Button(serial_started ? "Desconectar" : "Conectar", selected_port.empty()))
				toggle_serial();

			if (ImPlot::BeginPlot("Señal")) {
				//ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImGuiCond_FirstUseEver);
				//ImPlot::SetupAxisLimits(ImAxis_X1, -2, 2, ImGuiCond_Always);
				ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
				ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"s");
				//ImGui::BeginTooltip
				ImPlot::SetupAxisLimits(ImAxis_Y1, -8, 8, ImGuiCond_FirstUseEver);
				ImPlot::SetupAxisLimits(ImAxis_X1, left_limit, right_limit, serial_started && avanzar ? ImGuiCond_Always : ImGuiCond_None);
				//ImPlot::PlotBars("My Bar Plot", bar_data, 11);
				//ImPlot::PushStyleVar(legendshow, 0);
				ImPlot::PlotLine("Entrada", xs, ys, draw_size, ImPlotItemFlags_NoLegend);
				ImPlot::EndPlot();
			}

			if (ImPlot::BeginPlot("Fourier")) {
				ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
				ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"Hz");
				ImPlot::SetupAxis(ImAxis_X1, "Frecuencia", ImPlotAxisFlags_AutoFit);
				ImPlot::SetupAxis(ImAxis_Y1, "Amplitud", ImPlotAxisFlags_AutoFit);
				//ImPlot::PlotBars("My Bar Plot", bar_data, 11);
				//ImPlot::PushStyleVar(legendshow, 0);

				ImPlot::PlotStems("Entrada", fourier_x.data(), fourier.data(), out_N, 0, ImPlotItemFlags_NoLegend);
				ImPlot::EndPlot();
			}

			ImGui::SliderFloat("Time scale", &show_time, max_time_scale, min_time_scale);
			if (ImGui::Button(avanzar ? "Parar" : "Reanudar"))
				avanzar = !avanzar;

			ImGui::SliderInt("Máximo", &maximo, 0, 256);
			ImGui::SliderInt("Mínimo", &minimo, 0, 256);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();

			//ImGui::ShowDemoWindow();
			//ImPlot::ShowDemoWindow();
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}
#ifdef __EMSCRIPTEN__
	EMSCRIPTEN_MAINLOOP_END;
#endif

	run_receive = false;
	fetch_thread.join();

	destroy_buffers();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}