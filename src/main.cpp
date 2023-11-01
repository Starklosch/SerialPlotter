// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp


#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <thread>
#include <queue>
#include <fstream>
#include <chrono>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>
#include <imgui_internal.h>

#include "Serial.h"
#include "Buffers.h"
#include "Monitor.h"
#include "FFT.h"

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

float width = 1280, height = 720;

void window_resize(GLFWwindow* window, int w, int h) {
	width = w;
	height = h;
}

int MetricFormatter(double value, char* buff, int size, void* data) {
	const char* unit = (const char*)data;
	static double v[] = { 1e12, 1e9, 1e6, 1e3, 1, 1e-3, 1e-6, 1e-9, 1e-12};
	static const char* p[] = { "T", "G", "M", "k", "", "m", "u", "n", "p" };

	if (value == 0) {
		return snprintf(buff, size, "0 %s", unit);
	}

	//double exponent = log10(abs(value));
	//if (exponent < 12 && exponent > -12) {
	//	int i = (12 - exponent) / 3;
	//	return snprintf(buff, size, "%g %s%s", value / v[i], p[i], unit);
	for (int i = 0; i < 7; ++i) {
		if (fabs(value) >= v[i]) {
			return snprintf(buff, size, "%g %s%s", value / v[i], p[i], unit);
		}
	}
	return snprintf(buff, size, "%g %s%s", value / v[std::size(v) - 1], p[std::size(p) - 1], unit);
}

using namespace std::chrono_literals;
using namespace std::string_literals;

bool Button(const char* label, bool disabled = false) {
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

class Plott {
	std::vector<double> xs, ys;
	//std::vector<T> shownData;
	std::string title;
	size_t size;

	double min_hlim, max_hlim, min_vlim, max_vlim;
	ImPlotCond cond_hlim = ImPlotCond_Once, cond_vlim = ImPlotCond_Once;

	double* min_hlink = nullptr, * max_hlink = nullptr,
		* min_vlink = nullptr, * max_vlink = nullptr;

public:
	Plott(const char* title, size_t max) : title(title), xs(max), ys(max) {
	}

	void SetHLimits(double min_lim, double max_lim, ImPlotCond cond) {
		min_hlim = min_lim;
		max_hlim = max_lim;
		cond_hlim = cond;
	}

	void SetVLimits(double min_lim, double max_lim, ImPlotCond cond) {
		min_vlim = min_lim;
		max_vlim = max_lim;
		cond_vlim = cond;
	}

	void SetHLinks(double* min_link, double* max_link) {
		min_hlink = min_link;
		max_hlink = max_link;
	}

	void SetVLinks(double* min_link, double* max_link) {
		min_vlink = min_link;
		max_vlink = max_link;
	}

	void SetData(const double* input_xs, const double* input_ys, size_t size) {
		std::copy(input_xs, input_xs + size, xs.data());
		std::copy(input_ys, input_ys + size, ys.data());
		this->size = size;
	}

	void Draw() {
		if (!ImPlot::BeginPlot(title.c_str(), { -1,300 }))
			return;

		ImPlot::SetupAxisLinks(ImAxis_X1, min_hlink, max_hlink);
		ImPlot::SetupAxisLinks(ImAxis_Y1, min_vlink, max_vlink);

		ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
		ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"s");

		ImPlot::SetupAxisLimits(ImAxis_Y1, min_vlim, max_vlim, cond_vlim);
		ImPlot::SetupAxisLimits(ImAxis_X1, min_hlim, max_hlim, cond_hlim);

		ImPlot::PlotLine("", xs.data(), ys.data(), size, ImPlotItemFlags_NoLegend);
		ImPlot::EndPlot();
	}
};

void write_test() {
	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
	using duration = std::chrono::duration<double>;

	double f = 4, pi = 3.141592653589793;

	Monitor monitor(16 * 1024, 4 * 1024, 1024);
	monitor.start("COM5", 38400);

	time_point start = clock::now(), end = start + 1s;

	while (clock::now() < end) {
		double t = duration(clock::now() - start).count();
		double v = 255/2.f * (sin(2 * pi * f * t) + 1);
		monitor.put((uint8_t)v);
	}
}

// Main code
int main(int, char**)
{
	//write_test();
	//return 0;

	using clock = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;
	using duration = std::chrono::duration<double>;

	const int muestras = 3840;
	float frecuencia_muestreo = 3840;

	FFT fft(muestras);

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

	if (!gladLoadGL()) {
		glfwTerminate();
		return -1;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	const float e = 2.71828182846f;

	int selected_baud = frecuencia_muestreo * 10;

	int max_time = 60;
	int read_frequency = 30;
	float factor = 1;//pow(e, -(baud - 9600) / 200000.f);

	float show_time = 5 * factor;
	float offset = 0;// -100e-3;

	float max_time_visible = 10 * factor;
	float max_time_scale = 10.0f;
	float min_time_scale = 5e-4;

	//std::cout << std::format("Factor {} Max {} Min {} Show_time {}\n", factor, max_time_scale, min_time_scale, show_time);

	int speed, max_size, view_size = 0;
	size_t size = 0;

	std::vector<uint8_t> read_buffer, write_buffer;

	//Plott* input;
	ScrollBuffer<double>* scrollX = nullptr, * scrollY = nullptr;
	ScrollBuffer<double>* out_scrollX = nullptr, * out_scrollY = nullptr;
	float* xs = 0, * ys = 0;
	float next_time = 0;

	const auto destroy_buffers = [&] {
		//delete input;
		delete scrollX;
		delete scrollY;
		delete out_scrollX;
		delete out_scrollY;
		delete[] xs;
		delete[] ys;
	};

	const auto init_buffers = [&] {
		speed = selected_baud / 10;
		max_size = speed * max_time;
		size = 0;
		view_size = max_time_visible * speed;
		next_time = 0;
		//std::cout << std::format("Speed: {}\n", speed);

		destroy_buffers();
		read_buffer.resize(1024);
		write_buffer.resize(1024);
		//input = new Plott("Entrada", view_size);
		scrollX = new ScrollBuffer<double>(max_size, view_size);
		scrollY = new ScrollBuffer<double>(max_size, view_size);
		out_scrollX = new ScrollBuffer<double>(max_size, view_size);
		out_scrollY = new ScrollBuffer<double>(max_size, view_size);
		xs = new float[view_size];
		ys = new float[view_size];
	};

	init_buffers();

	Monitor monitor(16 * 1024, 4 * 1024, 1024);
	bool serial_started = false;
	bool show = true;

	std::string selected_port;

	time_point start_time = clock::now();

	bool avanzar = true;
	int draw_size = 0;
	int maximo = 0, minimo = 255;

	//std::ofstream log("C:/Users/usuario/Desktop/log.txt");

	std::function<double(double)> filtro = [](double d) {
		return d / 2;
	};
	const std::function<double(uint8_t)> transformacion = [&](uint8_t v) {
		return (v - minimo) / (double)(maximo - minimo) * 12 - 6;
	};
	const std::function<int(double)> transformacion_inversa = [&](double v) {
		double resultado = round((v + 6) * (maximo - minimo) / 12.0 + minimo);
		if (resultado < 0)
			return 0;
		if (resultado > 255)
			return 255;
		return (int)resultado;
	};

	int max = 0;
	monitor.read_callback = [&](int available) {
		int read = monitor.read(read_buffer.data(), read_buffer.size());

		for (size_t i = 0; i < read; i++)
		{
			double transformado = transformacion(read_buffer[i]);

			scrollY->push(transformado);
			scrollX->push(next_time);

			double resultado = filtro ? transformado : filtro(transformado);

			// Filtro
			out_scrollY->push(resultado);
			out_scrollX->push(next_time);
			next_time += 1 / frecuencia_muestreo;

			write_buffer[i] = transformacion_inversa(resultado);
		}

		monitor.write(write_buffer.data(), read);
		//log.write((char*)read_buffer.data(), read);

		size = scrollX->count();
		//if (available > max)
		//	max = available;
		//if (read < available)
		//	std::cout << "El buffer quedó chico\n";
	};

	double left_limit = 0, right_limit = max_time_visible;
	double ymin = -7, ymax = 7;
	double* linked_xmin = &left_limit, * linked_xmax = &right_limit;
	double* linked_ymin = &ymin, * linked_ymax = &ymax;

	//input->SetHLinks(linked_xmin, linked_xmax);
	//input->SetVLinks(linked_ymin, linked_ymax);
	//input->SetVLimits(-7, 7, ImPlotCond_Once);

	auto toggle_serial = [&] {
		if (!serial_started) {
			if (selected_port.empty())
				return;

			left_limit = 0, right_limit = max_time_visible;
			init_buffers();
			monitor.start(selected_port.c_str(), selected_baud);
			start_time = clock::now();
			scrollX->clear();
			scrollY->clear();
			linked_xmin = linked_xmax = nullptr;
		}
		else {
			monitor.stop();
			//std::cout << std::format("Máxima transferencia de bytes: {}\n", max);
			linked_xmin = &left_limit;
			linked_xmax = &right_limit;
		}
		serial_started = !serial_started;
	};

	ImPlotTransform scale = [](double v, void*) {
		return sqrt(v);
	};

	ImPlotTransform inverse_scale = [](double value, void*) {
		return value * value;
	};

	// Main loop
	while (!glfwWindowShouldClose(window))
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

		{
			auto puertos = EnumerateComPorts();

			static int stride = 4;
			static double elapsed_time = 0;
			int byte_stride = sizeof(double) * stride;

			draw_size = size / stride;
			if (serial_started) {
				elapsed_time = duration(clock::now() - start_time).count();

				if (elapsed_time > max_time_visible) {
					right_limit = scrollX->back();
					left_limit = right_limit - max_time_visible;
				}
			}

			ImGui::SetNextWindowPos({ 0, 0 });
			ImGui::SetNextWindowSize({ width, height });
			ImGui::Begin("Ventana principal", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar);

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Dispositivo")) {
					if (puertos.empty()) {
						ImGui::Text("No hay ningún dispositivo conectado");
					}
					else {
						for (const auto& puerto : puertos)
							if (ImGui::MenuItem(puerto.c_str()))
								selected_port = puerto;
					}
					ImGui::EndMenu();
				}

				if (Button(serial_started ? "Desconectar" : "Conectar", selected_port.empty())) {
					toggle_serial();
				}
				if (selected_port.empty()) {
					ImGui::SetItemTooltip("Selecciona un dispositivo primero");
				}
				ImGui::EndMenuBar();
			}

			ImGui::Text("Tiempo transcurrido: %.1fs", elapsed_time);
			ImGui::Text("Application average %.2f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::SliderInt("Stride", &stride, 1, 32);

			if (ImPlot::BeginAlignedPlots("Grupo")) {
				if (ImPlot::BeginPlot("Entrada", { -1,0 }, ImPlotFlags_NoLegend)) {
					ImPlot::SetupAxisLinks(ImAxis_X1, linked_xmin, linked_xmax);
					ImPlot::SetupAxisLinks(ImAxis_Y1, linked_ymin, linked_ymax);

					ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
					ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"s");

					ImPlot::SetupAxisLimits(ImAxis_Y1, -7, 7, ImGuiCond_FirstUseEver);
					ImPlot::SetupAxisLimits(ImAxis_X1, left_limit, right_limit, serial_started ? ImGuiCond_Always : ImGuiCond_None);

					//PlotStairs
					//ImPlot::DragRect(0, &rect.X.Min, &rect.Y.Min, &rect.X.Max, &rect.Y.Max, ImVec4(1, 0, 1, 1), flags);
					ImPlot::PlotLine("", scrollX->data(), scrollY->data(), draw_size, 0, 0, byte_stride);
					ImPlot::EndPlot();
				}

				//if (ImPlot::BeginPlot("##rect", ImVec2(-1, 0), ImPlotFlags_CanvasOnly)) {
				//	ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
				//	ImPlot::SetupAxesLimits(rect.X.Min, rect.X.Max, rect.Y.Min, rect.Y.Max, ImGuiCond_Always);
				//	ImPlot::PlotLine("Entrada", scrollX->data(), scrollY->data(), draw_size, ImPlotItemFlags_NoLegend);
				//	ImPlot::EndPlot();
				//}

				//if (ImPlot::BeginPlot("Salida", { -1,0 }, ImPlotFlags_NoLegend)) {
				//	ImPlot::SetupAxisLinks(ImAxis_X1, linked_xmin, linked_xmax);
				//	ImPlot::SetupAxisLinks(ImAxis_Y1, linked_ymin, linked_ymax);

				//	ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
				//	ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"s");
				//	ImPlot::SetupAxisLimits(ImAxis_Y1, -7, 7, ImGuiCond_FirstUseEver);
				//	ImPlot::SetupAxisLimits(ImAxis_X1, left_limit, right_limit, serial_started ? ImGuiCond_Always : ImGuiCond_None);

				//	ImPlot::PlotLine("", out_scrollX->data(), out_scrollY->data(), draw_size, 0, 0, byte_stride);
				//	ImPlot::EndPlot();
				//}
				ImPlot::EndAlignedPlots();
			}

			if (ImGui::CollapsingHeader("Análisis")) {
				if (ImPlot::BeginPlot("Espectro", { -1, 0 }, ImPlotFlags_NoLegend)) {
					std::async([&] {
						fft.SetData(scrollY->data(), muestras);
						fft.Compute();
					});

					ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"V");
					ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"Hz");
					ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_AutoFit);
					ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);

					//ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
					//ImPlot::SetupAxisScale(ImAxis_Y1, scale, inverse_scale);

					fft.Draw(frecuencia_muestreo);
					ImPlot::EndPlot();
				}
			}

			ImGui::SliderInt("Máximo", &maximo, 0, 256);
			ImGui::SliderInt("Mínimo", &minimo, 0, 256);

			ImGui::Text("Application average %.2f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		glViewport(0, 0, width, height);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}
#ifdef __EMSCRIPTEN__
	EMSCRIPTEN_MAINLOOP_END;
#endif

	monitor.stop();
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
