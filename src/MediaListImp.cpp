/*
 * Copyright 2010-2013 Esrille Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MediaListImp.h"

#include <assert.h>
#include <math.h>

#include <algorithm>
#include <utility>

#include <org/w3c/dom/css/CSSPrimitiveValue.h>

#include "css/CSSParser.h"
#include "css/CSSPropertyValueImp.h"
#include "css/ViewCSSImp.h"
#include "WindowProxy.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

using namespace stylesheets;

namespace {

const char16_t* const allType = u"all";

const char16_t* const mediaTypes[] = {
    u"braille",
    u"embossed",
    u"handheld",
    u"print",
    u"projection",
    u"screen",
    u"speech",
    u"tty",
    u"tv"
};

const char16_t* const featureTypes[] = {
    u"width",
    u"height",
    u"device-width",
    u"device-height",
    u"orientation",
    u"aspect-ratio",
    u"device-aspect-ratio",
    u"color",
    u"color-index",
    u"monochrome",
    u"resolution",
    u"scan",
    u"grid",
};

const char16_t* const orientationTypes[] = {
    u"",
    u"portrait",
    u"landscape",
};

const char16_t* const scanTypes[] = {
    u"",
    u"progressive",
    u"interlace",
};

const size_t mediaTypesCount = sizeof mediaTypes / sizeof mediaTypes[0];

}

bool MediaListImp::
MediaFeature::operator==(const MediaFeature& other) const {
    if (feature != other.feature)
        return false;
    if (unit != other.unit)
        return false;
    if (isnan(value) && isnan(other.value))
        return true;
    return value == other.value;
}

std::u16string MediaListImp::
MediaFeature::getMediaText() const
{
    std::u16string text;

    if (isMax())
        text += u"max-";
    else if (isMin())
        text += u"min-";
    text += featureTypes[feature & 0xfff];

    switch (feature) {
    case AspectRatio:
    case MinAspectRatio:
    case MaxAspectRatio:
    case DeviceAspectRatio:
    case MinDeviceAspectRatio:
    case MaxDeviceAspectRatio:
        if (!isnan(value))
            text += u": " + CSSSerializeNumber(value) + u'/' + CSSSerializeNumber(unit);
        break;
    case Orientation:
        if (0 < getIndex()) {
            text += u": ";
            text += orientationTypes[getIndex()];
        }
        break;
    case Scan:
        if (0 < getIndex())
            text += u": ";
            text += scanTypes[getIndex()];
        break;
    default:
        if (!isnan(value)) {
            text += u": " + CSSSerializeNumber(value);
            if (css::CSSPrimitiveValue::CSS_PERCENTAGE <= unit && unit <= css::CSSPrimitiveValue::CSS_KHZ)
                text += CSSNumericValue::Units[unit - css::CSSPrimitiveValue::CSS_PERCENTAGE];
            else if (css::CSSPrimitiveValue::CSS_DPPX <= unit && unit <= css::CSSPrimitiveValue::CSS_DPCM)
                text += CSSNumericValue::ResolutionUnits[unit - css::CSSPrimitiveValue::CSS_DPPX];
        }
        break;
    }

    return text;
}

MediaListImp::
MediaQuery::MediaQuery(unsigned type, std::list<MediaFeature>&& features) :
    type(type),
    features(std::move(features))
{
}

MediaListImp::
MediaQuery::MediaQuery(MediaQuery&& from) :
    type(from.type),
    features(std::move(from.features))
{
    from.type = 0;
    assert(from.features.empty());
}

bool MediaListImp::
MediaQuery::operator==(const MediaQuery& other) const
{
    if (type != other.type)
        return false;
    if (features.size() != other.features.size())
        return false;
    for (auto i = features.begin(); i != features.end(); ++i) {
        bool found = false;
        for (auto j = other.features.begin(); j != other.features.end(); ++j) {
            if (*i == *j) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

std::u16string MediaListImp::
MediaQuery::getMediaText()
{
    std::u16string text;

    if (type & Only)
        text += u"only ";
    if (type & Not)
        text += u"not ";
    if ((type & All) == All)
        text += allType;
    else {
        for (unsigned i = 0; i < mediaTypesCount; ++i) {
            if (type & (1u << i)) {
                text += mediaTypes[i];
                break;
            }
        }
    }
    for (auto i = features.begin(); i != features.end(); ++i)
        text += u" and (" + i->getMediaText() + u')';
    return text;
}

// MediaList

bool MediaListImp::matches(const WindowProxyPtr& window)
{
    if (!window)
        return false;
    if (mediaQueries.empty())
        return true;
    for (auto i = mediaQueries.begin(); i != mediaQueries.end(); ++i) {
        bool result = false;
        if (i->type & MediaListImp::Screen) {   // TODO: Support other medium
            result = true;
            // Check features
            for (auto j = i->features.begin(); j != i->features.end(); ++j) {
                switch (j->feature) {
                case Width:
                    if (j->unit) {
                        if (window->getInnerWidth() != window->getPx(j->value, j->unit))
                            result = false;
                    } else if (window->getInnerWidth() == 0)
                        result = false;
                    break;
                case MinWidth:
                    if (window->getInnerWidth() < window->getPx(j->value, j->unit))
                        result = false;
                    break;
                case MaxWidth:
                    if (window->getPx(j->value, j->unit) < window->getInnerWidth())
                        result = false;
                    break;
                case Height:
                    if (j->unit) {
                        if (window->getInnerHeight() != window->getPx(j->value, j->unit))
                            result = false;
                    } else if (window->getInnerHeight() == 0)
                        result =false;
                    break;
                case MinHeight:
                    if (window->getInnerHeight() < window->getPx(j->value, j->unit))
                        result = false;
                    break;
                case MaxHeight:
                    if (window->getPx(j->value, j->unit) < window->getInnerHeight())
                        result = false;
                    break;
                case DeviceWidth:
                    if (j->unit) {
                        if (window->getScreenImp()->getWidth() != window->getPx(j->value, j->unit))
                            result = false;
                    } else if (window->getScreenImp()->getWidth() == 0)
                        result = false;
                    break;
                case MinDeviceWidth:
                    if (window->getScreenImp()->getWidth() < window->getPx(j->value, j->unit))
                        result = false;
                    break;
                case MaxDeviceWidth:
                    if (window->getPx(j->value, j->unit) < window->getScreenImp()->getWidth())
                        result = false;
                    break;
                case DeviceHeight:
                    if (j->unit) {
                        if (window->getScreenImp()->getHeight() != window->getPx(j->value, j->unit))
                            result = false;
                    } else if (window->getScreenImp()->getHeight() == 0)
                        result = false;
                    break;
                case MinDeviceHeight:
                    if (window->getScreenImp()->getHeight() < window->getPx(j->value, j->unit))
                        result = false;
                    break;
                case MaxDeviceHeight:
                    if (window->getPx(j->value, j->unit) < window->getScreenImp()->getHeight())
                        result = false;
                    break;
                case Orientation:
                    switch (j->getIndex()) {
                    case Portrait:
                        if (window->getInnerHeight() < window->getInnerWidth())
                            result = false;
                        break;
                    case Landscape:
                        if (window->getInnerWidth() <= window->getInnerHeight())
                            result = false;
                        break;
                    default:
                        break;
                    }
                    break;
                case AspectRatio:
                    if (!isnan(j->value) && window->getInnerWidth() * j->unit != window->getInnerHeight() * j->value)
                        result = false;
                    break;
                case MinAspectRatio:
                    if (window->getInnerWidth() * j->unit < window->getInnerHeight() * j->value)
                        result = false;
                    break;
                case MaxAspectRatio:
                    if (window->getInnerHeight() * j->value < window->getInnerWidth() * j->unit)
                        result = false;
                    break;
                case DeviceAspectRatio:
                    if (!isnan(j->value) && window->getScreenImp()->getWidth() * j->unit != window->getScreenImp()->getHeight() * j->value)
                        result = false;
                    break;
                case MinDeviceAspectRatio:
                    if (window->getScreenImp()->getWidth() * j->unit < window->getScreenImp()->getHeight() * j->value)
                        result = false;
                    break;
                case MaxDeviceAspectRatio:
                    if (window->getScreenImp()->getHeight() * j->value < window->getScreenImp()->getWidth() * j->unit)
                        result = false;
                    break;
                case Color:
                    if (j->unit) {
                        if (window->getScreenImp()->getColor() != j->value)
                            result = false;
                    } else if (window->getScreenImp()->getColor() == 0)
                        result = false;
                    break;
                case MinColor:
                    if (window->getScreenImp()->getColor() < j->value)
                        result = false;
                    break;
                case MaxColor:
                    if (j->value < window->getScreenImp()->getColor())
                        result = false;
                    break;
                case ColorIndex:
                    if (j->unit) {
                        if (window->getScreenImp()->getColorIndex() != j->value)
                            result = false;
                    } else if (window->getScreenImp()->getColorIndex() == 0)
                        result = false;
                    break;
                case MinColorIndex:
                    if (window->getScreenImp()->getColorIndex() < j->value)
                        result = false;
                    break;
                case MaxColorIndex:
                    if (j->value < window->getScreenImp()->getColorIndex())
                        result = false;
                    break;
                case Monochrome:
                    if (j->unit) {
                        if (window->getScreenImp()->getMonochrome() != j->value)
                            result = false;
                    } else if (window->getScreenImp()->getMonochrome() == 0)
                        result = false;
                    break;
                case MinMonochrome:
                    if (window->getScreenImp()->getMonochrome() < j->value)
                        result = false;
                    break;
                case MaxMonochrome:
                    if (j->value < window->getScreenImp()->getMonochrome())
                        result = false;
                    break;
                case Resolution:
                    switch (j->unit) {
                    case css::CSSPrimitiveValue::CSS_DPI:
                    case css::CSSPrimitiveValue::CSS_DPCM:
                        if (window->getScreenImp()->getDPI() != window->getPx(j->value, j->unit))
                            result = false;
                        break;
                    case css::CSSPrimitiveValue::CSS_DPPX:
                        if (window->getScreenImp()->getDPPX() != j->value)
                            result = false;
                        break;
                    default:
                        break;
                    }
                    break;
                case MinResolution:
                    switch (j->unit) {
                    case css::CSSPrimitiveValue::CSS_DPI:
                    case css::CSSPrimitiveValue::CSS_DPCM:
                        if (window->getScreenImp()->getDPI() < window->getPx(j->value, j->unit))
                            result = false;
                        break;
                    case css::CSSPrimitiveValue::CSS_DPPX:
                        if (window->getScreenImp()->getDPPX() < j->value)
                            result = false;
                        break;
                    default:
                        break;
                    }
                    break;
                case MaxResolution:
                    switch (j->unit) {
                    case css::CSSPrimitiveValue::CSS_DPI:
                    case css::CSSPrimitiveValue::CSS_DPCM:
                        if (window->getPx(j->value, j->unit) < window->getScreenImp()->getDPI())
                            result = false;
                        break;
                    case css::CSSPrimitiveValue::CSS_DPPX:
                        if (j->value < window->getScreenImp()->getDPPX())
                            result = false;
                        break;
                    default:
                        break;
                    }
                    break;
                case Scan:
                    if (i->type != Tv)
                        result = false;
                    else if (j->getIndex() && j->getIndex() != window->getScreenImp()->getScan())
                        result = false;
                    break;
                case Grid:
                    if (j->unit) {
                        if (j->value != window->getScreenImp()->getGrid())
                            result = false;
                    } else if (window->getScreenImp()->getGrid() == 0)
                            result = false;
                    break;
                default:
                    result = false;
                    break;
                }
                if (!result)
                    break;
            }
        }
        if (i->type & Not)
            result ^= 1;
        if (result)
            return true;
    }
    return false;
}

bool MediaListImp::contains(const MediaQuery& mediaQuery) const
{
    return std::find(mediaQueries.begin(), mediaQueries.end(), mediaQuery) != mediaQueries.end();
}

void MediaListImp::appendMedium(unsigned media)
{
    MediaQuery mediaQuery(media, std::move(mediaFeatures));
    assert(mediaFeatures.empty());
    for (auto i = mediaQuery.features.begin(); i != mediaQuery.features.end(); ++i) {
        if (i->feature == Unknown) {
            mediaQuery.type = Not | All;
            mediaQuery.features.clear();
            break;
        }
    }
    if (!contains(mediaQuery))
        mediaQueries.push_back(std::move(mediaQuery));
}

void MediaListImp::appendFeature(int feature, CSSParserExpr* expr)
{
    float value(NAN);
    unsigned short unit(css::CSSPrimitiveValue::CSS_UNKNOWN);    // or index
    if (expr) {
        CSSValueParser parser(feature);
        if (!parser.isValid(expr))
            feature = Unknown;
        else {
            CSSParserTerm* term = parser.getStack().back();
            switch (feature) {
            case AspectRatio:
            case MinAspectRatio:
            case MaxAspectRatio:
            case DeviceAspectRatio:
            case MinDeviceAspectRatio:
            case MaxDeviceAspectRatio:
                assert(parser.getStack().size() == 3);
                value = parser.getStack()[0]->getNumber();  // width
                unit = parser.getStack()[2]->getNumber();   // height
                break;
            case Orientation:
            case Scan:
                unit = (term->getIndex() < 0) ? 0 : term->getIndex();
                break;
            default:
                value = term->getNumber();
                unit = term->unit;
                break;
            }
        }
        delete expr;
    } else if (feature & Min)   // is min or is max?
        feature = Unknown;
    mediaFeatures.emplace_back(feature, value, unit);
}

std::u16string MediaListImp::getMediaText()
{
    if (mediaQueries.empty())
        return allType;
    std::u16string text;
    for (auto i = mediaQueries.begin(); i != mediaQueries.end(); ++i) {
        if (i != mediaQueries.begin())
            text += u", ";
        text += i->getMediaText();
    }
    return text;
}

void MediaListImp::setMediaText(const std::u16string& mediaText)
{
    clear();
    if (!mediaText.empty()) {
        CSSParser parser;
        parser.setMediaList(std::static_pointer_cast<MediaListImp>(self()));
        parser.parseMediaList(mediaText);
        parser.setMediaList(nullptr);
    }
}

unsigned int MediaListImp::getLength()
{
    return mediaQueries.empty() ? 1 : mediaQueries.size();
}

std::u16string MediaListImp::item(unsigned int index)
{
    if (index < getLength()) {
        if (mediaQueries.empty())
            return allType;
        for (auto i = mediaQueries.begin(); i != mediaQueries.end(); ++i) {
            if (index == 0)
                return i->getMediaText();
            --index;
        }
    }
    return u"";
}

void MediaListImp::appendMedium(const std::u16string& medium)
{
    if (medium.empty())
        return;
    CSSParser parser;
    MediaListPtr newList = std::make_shared<MediaListImp>();
    parser.setMediaList(newList);
    parser.parseMediaList(medium);
    for (auto i = newList->mediaQueries.begin(); i != newList->mediaQueries.end(); ++i) {
        if (!contains(*i))
            mediaQueries.push_back(std::move(*i));
    }
    parser.setMediaList(nullptr);
}

void MediaListImp::deleteMedium(const std::u16string& medium)
{
    if (medium.empty())
        return;
    CSSParser parser;
    MediaListPtr newList = std::make_shared<MediaListImp>();
    parser.setMediaList(newList);
    parser.parseMediaList(medium);
    for (auto i = newList->mediaQueries.begin(); i != newList->mediaQueries.end(); ++i) {
        auto found = std::find(mediaQueries.begin(), mediaQueries.end(), *i);
        if (found != mediaQueries.end())
            mediaQueries.erase(found);
    }
    parser.setMediaList(nullptr);
}

}  // org::w3c::dom::bootstrap

namespace stylesheets {

namespace {

class Constructor : public Object
{
public:
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv) {
        bootstrap::MediaListPtr imp;
        switch (argc) {
        case 1:
            imp = std::make_shared<bootstrap::MediaListImp>();
            if (imp)
                imp->setMediaText(argv[0].toString());
            break;
        default:
            break;
        }
        return imp;
    }
    Constructor() :
        Object(this) {
    }
};

}  // namespace

Object MediaList::getConstructor()
{
    static Constructor constructor;
    return constructor.self();
}

}

}}}  // org::w3c::dom::bootstrap
