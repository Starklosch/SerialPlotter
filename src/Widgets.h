#pragma once
#include <functional>
#include <imgui_internal.h>


template <class Container>
void select_menu(const char* title, typename Container::value_type& selection, std::function<Container()> get_values, const std::function<std::string(typename Container::value_type)>& to_string, const char* empty_msg = "Vacio") {
    if (!ImGui::BeginMenu(title))
        return;

    Container values = get_values();
    if (std::empty(values)) {
        ImGui::Text(empty_msg);
        ImGui::EndMenu();
        return;
    }

    bool selected = true;
    for (const auto& value : values) {
        auto str = to_string(value);
        if (ImGui::MenuItem(str.c_str(), nullptr, value == selection ? &selected : nullptr)) {
            selection = value;
        }
    }

    ImGui::EndMenu();
}

template <class Container, typename T>
void combo(const char* title, T& selection, const Container& values, const std::function<std::string(T)>& to_string, const char* empty_msg = "Vacio") {
    auto str = to_string(selection);
    if (!ImGui::BeginCombo(title, str.c_str()))
        return;

    if (std::empty(values)) {
        ImGui::Text(empty_msg);
        ImGui::EndCombo();
        return;
    }

    for (const auto& value : values) {
        auto str = to_string(value);
        bool isSelected = value == selection;
        if (ImGui::Selectable(str.c_str(), &isSelected)) {
            selection = value;
        }
    }

    ImGui::EndCombo();
}