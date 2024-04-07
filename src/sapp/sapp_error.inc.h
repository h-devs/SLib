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

#include "slib/core.h"

namespace {

	SLIB_STATIC_STRING(g_str_log_appconf_begin, "Opening app configuration file: %s")
	SLIB_STATIC_STRING(g_str_log_open_drawables_begin, "Opening drawable resources: %s")
	SLIB_STATIC_STRING(g_str_log_open_raws_begin, "Opening raw resources: %s")
	SLIB_STATIC_STRING(g_str_log_open_resource_begin, "Opening resource file: %s")
	SLIB_STATIC_STRING(g_str_log_generate_cpp_begin, "Generating C++ files: %s")
	SLIB_STATIC_STRING(g_str_error_generate_cpp_target_path_is_empty, "App configuration: <generate-cpp>/<target-path> is empty")
	SLIB_STATIC_STRING(g_str_error_generate_cpp_target_path_invalid, "App configuration: Failed to create target path for generating cpp: %s")
	SLIB_STATIC_STRING(g_str_error_generate_cpp_namespace_is_empty, "App configuration: <generate-cpp>/<namespace> is empty")
	SLIB_STATIC_STRING(g_str_error_generate_cpp_namespace_invalid, "App configuration: Invalid <generate-cpp>/<namespace>: %s")
	SLIB_STATIC_STRING(g_str_log_generate_cpp_resources_h_begin, "Generating: resources.h")

	SLIB_STATIC_STRING(g_str_error_configuration_value_empty, "App configuration: <%s> tag is empty")
	SLIB_STATIC_STRING(g_str_error_configuration_value_invalid, "App configuration: Value of <%s> tag is invalid: %s")

	SLIB_STATIC_STRING(g_str_error_out_of_memory, "Out of memory")
	SLIB_STATIC_STRING(g_str_error_directory_not_found, "Directory not found: %s")
	SLIB_STATIC_STRING(g_str_error_directory_create_failed, "Creating directory is failed: %s")
	SLIB_STATIC_STRING(g_str_error_file_not_found, "File not found: %s")
	SLIB_STATIC_STRING(g_str_error_file_write_failed, "Failed to write to file: %s")
	SLIB_STATIC_STRING(g_str_error_invalid_root_tag, "<sapp> tag is not declared for root element")
	SLIB_STATIC_STRING(g_str_error_configuration_tag_not_found, "<configuration> tag is not found in root element")
	SLIB_STATIC_STRING(g_str_error_invalid_tag, "Invalid tag: %s")
	SLIB_STATIC_STRING(g_str_error_string_not_found, "String resource is not defined: %s")
	SLIB_STATIC_STRING(g_str_error_color_not_found, "Color resource is not defined: %s")
	SLIB_STATIC_STRING(g_str_error_drawable_not_found, "Drawable resource is not defined: %s")
	SLIB_STATIC_STRING(g_str_error_drawable_not_image, "Drawable resource is not image: %s")
	SLIB_STATIC_STRING(g_str_error_load_image_failed, "Failed to load image drawable resource: %s")
	SLIB_STATIC_STRING(g_str_error_menu_not_found, "Menu resource is not defined: %s")
	SLIB_STATIC_STRING(g_str_error_load_menu_failed, "Failed to load menu resource: %s")
	SLIB_STATIC_STRING(g_str_error_layout_style_not_found, "LayoutStyle is not found: %s")
	SLIB_STATIC_STRING(g_str_error_layout_include_not_found, "LayoutInclude is not found: %s")
	SLIB_STATIC_STRING(g_str_error_layout_not_found, "Layout is not found: %s")
	SLIB_STATIC_STRING(g_str_error_layout_is_not_view, "Layout is not a view: %s")

	SLIB_STATIC_STRING(g_str_error_resource_raw_name_duplicated, "Raw Resource: Generated name %s is duplicated: %s")
	SLIB_STATIC_STRING(g_str_error_resource_raw_size_big, "Raw Resource: Size is larger than 16MB: %s")
	SLIB_STATIC_STRING(g_str_log_generate_cpp_raws_begin, "Generating: raws, raws.cpp")

	SLIB_STATIC_STRING(g_str_error_resource_drawable_locale_invalid, "Drawable Resource: Invalid locale is found in the drawable directory name: %s")
	SLIB_STATIC_STRING(g_str_error_resource_drawable_filename_invalid, "Drawable Resource: Invalid image filename: %s")
	SLIB_STATIC_STRING(g_str_error_resource_drawable_not_defined_default, "Drawable Resource: Default resource is not defined for this name: %s")
	SLIB_STATIC_STRING(g_str_error_resource_drawable_type_duplicated, "Drawable Resource: Invalid type is already defined on the name: %s")
	SLIB_STATIC_STRING(g_str_error_resource_drawable_load_image_failed, "Drawable Resource: Loading image file failed: %s")
	SLIB_STATIC_STRING(g_str_error_resource_ninepieces_name_is_empty, "Nine-Pieces Resource: `name` attribute is empty")
	SLIB_STATIC_STRING(g_str_error_resource_ninepieces_name_invalid, "Nine-Pieces Resource: Invalid `name` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_ninepieces_name_redefined, "Nine-Pieces Resource: `name` is redefined: %s")
	SLIB_STATIC_STRING(g_str_error_resource_ninepieces_attribute_invalid, "Nine-Pieces Resource: Invalid `%s` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_ninepatch_name_is_empty, "Nine-Patch Resource: `name` attribute is empty")
	SLIB_STATIC_STRING(g_str_error_resource_ninepatch_name_invalid, "Nine-Patch Resource: Invalid `name` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_ninepatch_name_redefined, "Nine-Patch Resource: `name` is redefined: %s")
	SLIB_STATIC_STRING(g_str_error_resource_ninepatch_attribute_invalid, "Nine-Patch Resource: Invalid `%s` attribute value: %s")
	SLIB_STATIC_STRING(g_str_log_generate_cpp_drawables_begin, "Generating: drawables.h, drawables.cpp")

	SLIB_STATIC_STRING(g_str_error_resource_string_locale_invalid, "String Resource: Invalid `locale` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_string_name_is_empty, "String Resource: `name` attribute is empty")
	SLIB_STATIC_STRING(g_str_error_resource_string_name_invalid, "String Resource: Invalid `name` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_string_redefine_default, "String Resource: Default value is redefined")
	SLIB_STATIC_STRING(g_str_error_resource_string_redefine_locale, "String Resource: Value is redefined for `%s` locale")
	SLIB_STATIC_STRING(g_str_log_generate_cpp_strings_begin, "Generating: strings.h, strings.cpp")

	SLIB_STATIC_STRING(g_str_error_resource_color_name_is_empty, "Color Resource: `name` attribute is empty")
	SLIB_STATIC_STRING(g_str_error_resource_color_name_invalid, "Color Resource: Invalid `name` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_color_name_redefined, "Color Resource: `name` is redefined: %s")
	SLIB_STATIC_STRING(g_str_error_resource_color_value_invalid, "Color Resource: Invalid color value: %s")
	SLIB_STATIC_STRING(g_str_log_generate_cpp_colors_begin, "Generating: colors.h, colors.cpp")

	SLIB_STATIC_STRING(g_str_error_resource_menu_name_is_empty, "Menu Resource: `name` attribute is empty")
	SLIB_STATIC_STRING(g_str_error_resource_menu_name_is_root, "Menu Resource: `root` is not allowed for item name")
	SLIB_STATIC_STRING(g_str_error_resource_menu_name_invalid, "Menu Resource: Invalid `name` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_menu_name_redefined, "Menu Resource: `name` is redefined: %s")
	SLIB_STATIC_STRING(g_str_error_resource_menu_title_refer_invalid, "Menu Resource: `title` should be valid string value or string resource: %s")
	SLIB_STATIC_STRING(g_str_error_resource_menu_checked_invalid, "Menu Resource: Invalid `checked` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_menu_icon_invalid, "Menu Resource: Invalid `icon` image resource: %s")
	SLIB_STATIC_STRING(g_str_error_resource_menu_platform_invalid, "Menu Resource: Invalid `platform` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_menu_shortcutKey_invalid, "Menu Resource: Invalid `shortcutKey` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_menu_macShortcutKey_invalid, "Menu Resource: Invalid `macShortcutKey` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_menu_children_tag_invalid, "Menu Resource: Invalid tag: %s")
	SLIB_STATIC_STRING(g_str_error_resource_menu_item_name_redefined, "Menu Resource: Item name is redefined: %s")
	SLIB_STATIC_STRING(g_str_log_generate_cpp_menus_begin, "Generating: menus.h, menus.cpp")

	SLIB_STATIC_STRING(g_str_error_resource_layout_type_invalid, "Layout Resource: Invalid layout type: %s")
	SLIB_STATIC_STRING(g_str_error_resource_layout_name_is_empty, "Layout Resource: `name` attribute is empty")
	SLIB_STATIC_STRING(g_str_error_resource_layout_name_invalid, "Layout Resource: Invalid `name` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_layout_name_redefined, "Layout Resource: `name` is redefined: %s")
	SLIB_STATIC_STRING(g_str_error_resource_layout_name_array_item_class_different, "Layout Resource: Different class is used for array item: %s")
	SLIB_STATIC_STRING(g_str_error_resource_layout_value_invalid, "Layout Resource: Invalid value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_layout_attribute_invalid, "Layout Resource: Invalid `%s` attribute value: %s")
	SLIB_STATIC_STRING(g_str_error_resource_layout_failed_load_reference_view, "Layout Resource: Failed to load reference view: %s")
	SLIB_STATIC_STRING(g_str_error_resource_layout_scrollview_must_contain_one_child, "Layout Resource: ScrollView can contain only one child")
	SLIB_STATIC_STRING(g_str_error_resource_layout_refreshview_must_contain_one_child, "Layout Resource: `RefreshView` can contain only one child")
	SLIB_STATIC_STRING(g_str_error_resource_layout_gridview_unknown_cell_creator, "Layout Resource: Unknown grid cell tag: %s")
	SLIB_STATIC_STRING(g_str_log_generate_cpp_layouts_begin, "Generating: layouts.h, layouts.cpp")
}
