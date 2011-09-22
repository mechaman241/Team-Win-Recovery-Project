// text.cpp - GUIText object

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <string>

extern "C" {
#include "../common.h"
#include "../minui/minui.h"
#include "../recovery_ui.h"
}

#include "rapidxml.hpp"
#include "objects.hpp"

GUIText::GUIText(xml_node<>* node)
{
    xml_attribute<>* attr;
    xml_node<>* child;

    mFont = NULL;
    mIsStatic = 1;
    mVarChanged = 0;
    mFontHeight = 0;

    if (!node)
    {
        LOGE("GUIText created without XML node\n");
        return;
    }

    // Initialize color to solid black
    memset(&mColor, 0, sizeof(COLOR));
    mColor.alpha = 255;

    attr = node->first_attribute("color");
    if (attr)
    {
        std::string color = attr->value();
        ConvertStrToColor(color, &mColor);
    }

    // Load the font, and possibly override the color
    child = node->first_node("font");
    if (child)
    {
        attr = child->first_attribute("resource");
        if (attr)
            mFont = PageManager::FindResource(attr->value());

        attr = child->first_attribute("color");
        if (attr)
        {
            std::string color = attr->value();
            ConvertStrToColor(color, &mColor);
        }
    }

    // Find the placement
    child = node->first_node("placement");
    if (child)
    {
        attr = child->first_attribute("x");
        if (attr)   mRenderX = atol(attr->value());

        attr = child->first_attribute("y");
        if (attr)   mRenderY = atol(attr->value());

        attr = child->first_attribute("w");
        if (attr)   mRenderW = atol(attr->value());

        attr = child->first_attribute("h");
        if (attr)   mRenderH = atol(attr->value());
    }

    child = node->first_node("text");
    if (child)  mText = child->value();
    else        LOGE("Text field created without any text\n");

    // Simple way to check for static state
    mLastValue = parseText();
    if (mLastValue != mText)   mIsStatic = 0;

    gr_getFontDetails(mFont ? mFont->GetResource() : NULL, (unsigned*) &mFontHeight, NULL);
    return;
}

int GUIText::Render(void)
{
    void* fontResource = NULL;

    if (mFont)  fontResource = mFont->GetResource();

    mLastValue = parseText();
    mVarChanged = 0;

    // TODO: width and height aren't bounded.
    gr_color(mColor.red, mColor.green, mColor.blue, mColor.alpha);
    gr_text(mRenderX, mRenderY, mLastValue.c_str(), fontResource);
    return 0;
}

int GUIText::Update(void)
{
    static int updateCounter = 15;

    // This hack just makes sure we update at least once a minute for things like clock and battery
    if (updateCounter)  updateCounter--;
    else
    {
        mVarChanged = 1;
        updateCounter = 15;
    }

    if (mIsStatic || !mVarChanged)      return 0;

    std::string newValue = parseText();
    if (mLastValue == newValue)         return 0;
    return 2;
}

int GUIText::GetCurrentBounds(int& w, int& h)
{
    void* fontResource = NULL;

    if (mFont)  fontResource = mFont->GetResource();

    h = mFontHeight;
    w = gr_measure(mLastValue.c_str(), fontResource);
    return 0;
}

std::string GUIText::parseText(void)
{
    static int counter = 0;
    std::string str = mText;
    size_t pos = 0;
    size_t next = 0, end = 0;

    while (1)
    {
        next = str.find('%', pos);
        if (next == std::string::npos)      return str;
        end = str.find('%', next + 1);
        if (end == std::string::npos)       return str;

        // We have a block of data
        std::string var = str.substr(next + 1, (end - next) - 1);
        str.erase(next, (end - next) + 1);

        if (next + 1 == end)
        {
            str.insert(next, 1, '%');
        }
        else
        {
            std::string value;
            if (DataManager::GetValue(var, value) == 0)
                str.insert(next, value);
        }

        pos = next + 1;
    }
}

int GUIText::NotifyVarChange(std::string varName, std::string value)
{
    mVarChanged = 1;
    return 0;
}

