/**
 * @file footer_item_multitool.hpp
 * @brief footer items for tools
 */

#pragma once
#include "ifooter_item.hpp"

class FooterItemFinda : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    static string_view_utf8 GetName();
    FooterItemFinda(window_t *parent);
};

class FooterItemCurrentTool : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    static string_view_utf8 GetName();
    FooterItemCurrentTool(window_t *parent);
};
