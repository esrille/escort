/*
 * Copyright 2011-2013 Esrille Inc.
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

#include "HTMLInputElementImp.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <org/w3c/dom/Text.h>
#include <org/w3c/dom/events/KeyboardEvent.h>
#include <org/w3c/dom/html/HTMLTemplateElement.h>

#include "one_at_a_time.hpp"

constexpr auto Intern = &one_at_a_time::hash<char16_t>;

#include "utf.h"
#include "DocumentImp.h"
#include "HTMLTemplateElementImp.h"
#include "HTMLUtil.h"
#include "css/CSSStyleDeclarationImp.h"     // TODO: only for XBL; isolate this later.

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{

namespace {

const char16_t* typeKeywords[] = {
    u"hidden",
    u"text",
    u"search",
    u"tel",
    u"url",
    u"email",
    u"password",
    u"datetime",
    u"date",
    u"month",
    u"week",
    u"time",
    u"datetime-local",
    u"number",
    u"range",
    u"color",
    u"checkbox",
    u"radio",
    u"file",
    u"submit",
    u"image",
    u"reset",
    u"button",
};

}  // namespace

HTMLInputElementImp::HTMLInputElementImp(DocumentImp* ownerDocument) :
    ObjectMixin(ownerDocument, u"input"),
    type(Text),
    clickListener(boost::bind(&HTMLInputElementImp::handleClick, this, _1, _2)),
    keydownListener(boost::bind(&HTMLInputElementImp::handleKeydown, this, _1, _2)),
    cursor(0),
    checked(false)
{
    tabIndex = 0;
}

HTMLInputElementImp::HTMLInputElementImp(const HTMLInputElementImp& org) :
    ObjectMixin(org),
    type(org.type),
    clickListener(boost::bind(&HTMLInputElementImp::handleClick, this, _1, _2)),
    keydownListener(boost::bind(&HTMLInputElementImp::handleKeydown, this, _1, _2)),
    cursor(0),
    checked(org.checked)
{
}

css::CSSStyleDeclaration HTMLInputElementImp::getStyle()
{
    if (hasStyle())
        return HTMLElementImp::getStyle();
    css::CSSStyleDeclaration style = HTMLElementImp::getStyle();
    if (auto imp = std::dynamic_pointer_cast<CSSStyleDeclarationImp>(style.self())) {
        switch (type) {
        case Text:
        case Search:
        case Telephone:
        case URL:
        case Email:
        case Password:
            imp->setProperty(u"width", boost::lexical_cast<std::u16string>(getSize()) + u"em", u"non-css");
            break;
        default:
            break;
        }
    }
    return style;
}

void HTMLInputElementImp::updateStyle()
{
    css::CSSStyleDeclaration style = HTMLElementImp::getStyle();
    if (auto imp = std::dynamic_pointer_cast<CSSStyleDeclarationImp>(style.self())) {
        switch (type) {
        case Text:
        case Search:
        case Telephone:
        case URL:
        case Email:
        case Password:
            imp->setProperty(u"height", u"", u"non-css");
            imp->setProperty(u"width", boost::lexical_cast<std::u16string>(getSize()) + u"em", u"non-css");
            break;
        case ImageButton: {
            std::u16string value;
            value = getAttribute(u"height");
            imp->setProperty(u"height", mapToPixelLength(value) ? value : u"", u"non-css");
            value = getAttribute(u"width");
            imp->setProperty(u"width", mapToPixelLength(value) ? value : u"", u"non-css");
            break;
        }
        default:
            imp->setProperty(u"height", u"", u"non-css");
            imp->setProperty(u"width", u"", u"non-css");
            break;
        }
    }
}

void HTMLInputElementImp::handleMutation(events::MutationEvent mutation)
{
    std::u16string value = mutation.getNewValue();
    css::CSSStyleDeclaration style(getStyle());

    switch (Intern(mutation.getAttrName().c_str())) {
    case Intern(u"checked"):
        // TODO: process dirty checkedness flag
        if (mutation.getAttrChange() == events::MutationEvent::REMOVAL)
            checked = false;
        else
            checked = true;
        break;
    case Intern(u"disabled"):
        if (mutation.getAttrChange() != events::MutationEvent::REMOVAL)
            tabIndex = -1;
        else if (type != Hidden && !toInteger(getAttribute(u"tabindex"), tabIndex))
            tabIndex = 0;
        break;
    case Intern(u"tabindex"):
        if (!getDisabled() && type != Hidden && !toInteger(getAttribute(u"tabindex"), tabIndex))
            tabIndex = 0;
        break;
    case Intern(u"type"):
        toLower(value);
        type = findKeyword(value, typeKeywords, TypeMax);
        if (type < 0)
            type = Text;
        if (type == Hidden)
            tabIndex = -1;
        else if (!getDisabled() && !toInteger(getAttribute(u"tabindex"), tabIndex))
            tabIndex = 0;
        updateStyle();
        break;
    // Styles
    case Intern(u"height"):
        if (type == ImageButton)
            style.setProperty(u"height", mapToPixelLength(value) ? value : u"", u"non-css");
        break;
    case Intern(u"hspace"):
        if (!mapToDimension(value))
            value = u"";
        style.setProperty(u"margin-left", value, u"non-css");
        style.setProperty(u"margin-right", value, u"non-css");
        break;
    case Intern(u"size"):
        switch (type) {
        case Text:
        case Search:
        case Telephone:
        case URL:
        case Email:
        case Password: {
            unsigned size;
            if (!toUnsigned(value, size))
                size = 20;
            style.setProperty(u"width",  boost::lexical_cast<std::u16string>(size) + u"em", u"non-css");
            break;
        }
        default:
            break;
        }
        break;
    case Intern(u"vspace"):
        if (!mapToDimension(value))
            value = u"";
        style.setProperty(u"margin-top", value, u"non-css");
        style.setProperty(u"margin-bottom", value, u"non-css");
        break;
    case Intern(u"width"):
        if (type == ImageButton)
            style.setProperty(u"width", mapToPixelLength(value) ? value : u"", u"non-css");
        break;
    default:
        HTMLElementImp::handleMutation(mutation);
        break;
    }
}

void HTMLInputElementImp::handleClick(EventListenerImp* listener, events::Event event)
{
    if (type == SubmitButton) {
        if (auto imp = std::dynamic_pointer_cast<HTMLFormElementImp>(getForm().self()))
            imp->submit(std::static_pointer_cast<HTMLInputElementImp>(self()));
    }
}

void HTMLInputElementImp::handleKeydown(EventListenerImp* listener, events::Event event)
{
    if (type == Text) {
        bool modified = false;
        std::u16string value = getValue();
        events::KeyboardEvent key = interface_cast<events::KeyboardEvent>(event);
        char16_t c = key.getCharCode();
        if (32 <= c && c < 127) {
            value.insert(cursor, 1, c);
            ++cursor;
            modified = true;
        } else if (c == 8) {
            if (0 < cursor) {
                --cursor;
                value.erase(cursor, 1);
                modified = true;
            }
        }
        unsigned k = key.getKeyCode();
        switch (k) {
        case 35:  // End
            cursor = value.length();
            break;
        case 36:  // Home
            cursor = 0;
            break;
        case 37:  // <-
            if (0 < cursor)
                --cursor;
            break;
        case 39:  // ->
            if (cursor < value.length())
                ++cursor;
            break;
        case 46:  // Del
            if (cursor < value.length()) {
                value.erase(cursor, 1);
                modified = true;
            }
            break;
        default:
            break;
        }
        if (modified)
            setValue(value);
    }
}

bool HTMLInputElementImp::generateShadowContent(const CSSStyleDeclarationPtr& style)
{
    if (style->display.getValue() == CSSDisplayValueImp::None || getShadowTree())
        return false;

    DocumentPtr document = getOwnerDocumentImp();
    assert(document);
    switch (style->binding.getValue()) {
    case CSSBindingValueImp::InputTextfield: {
        if (auto element = std::make_shared<HTMLTemplateElementImp>(document.get())) {
            dom::Text text = document->createTextNode(getValue());
            element->appendChild(text, true);
            style->setCssText(u"display: inline-block; white-space: pre; background-color: white; border: 2px inset; text-align: left; padding: 1px; min-height: 1em;");
            setShadowTree(element);
            addEventListener(u"keydown", keydownListener, false, EventTargetImp::UseDefault);
            return true;
        }
        break;
    }
    case CSSBindingValueImp::InputButton: {
        if (auto element = std::make_shared<HTMLTemplateElementImp>(document.get())) {
            dom::Text text = document->createTextNode(getValue());
            element->appendChild(text, true);
            style->setCssText(u"display: inline-block; border: 2px outset; padding: 1px; text-align: center; min-height: 1em;");
            setShadowTree(element);
            addEventListener(u"click", clickListener, false, EventTargetImp::UseDefault);
            return true;
        }
        break;
    }
    case CSSBindingValueImp::InputRadio: {
        if (auto element = std::make_shared<HTMLTemplateElementImp>(document.get())) {
            dom::Text text = document->createTextNode(getChecked() ? u"\u25c9" : u"\u25cb");
            element->appendChild(text, true);
            setShadowTree(element);
            return true;
        }
        break;
    }
    case CSSBindingValueImp::InputCheckbox: {
        if (auto element = std::make_shared<HTMLTemplateElementImp>(document.get())) {
            dom::Text text = document->createTextNode(getChecked() ? u"\u2611" : u"\u2610");
            element->appendChild(text, true);
            setShadowTree(element);
            return true;
        }
        break;
    }
    default:
        if (HTMLElementImp::generateShadowContent(style)) {
            switch (type) {
            case SubmitButton:
                addEventListener(u"click", clickListener, false, EventTargetImp::UseDefault);
                break;
            default:
                break;
            }
            return true;
        }
        break;
    }
    return false;
}

std::u16string HTMLInputElementImp::getAccept()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setAccept(const std::u16string& accept)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getAlt()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setAlt(const std::u16string& alt)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getAutocomplete()
{
    return getAttribute(u"autocomplete");
}

void HTMLInputElementImp::setAutocomplete(const std::u16string& autocomplete)
{
    setAttribute(u"autocomplete", autocomplete);
}

bool HTMLInputElementImp::getAutofocus()
{
    return getAttributeAsBoolean(u"autofocus");
}

void HTMLInputElementImp::setAutofocus(bool autofocus)
{
    setAttributeAsBoolean(u"autofocus", autofocus);
}

bool HTMLInputElementImp::getDefaultChecked()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setDefaultChecked(bool defaultChecked)
{
    // TODO: implement me!
}

bool HTMLInputElementImp::getChecked()
{
    // TODO: process dirty checkedness flag
    return checked;
}

void HTMLInputElementImp::setChecked(bool checked)
{
    // TODO: process dirty checkedness flag
    setAttributeAsBoolean(u"checked", checked);
}

std::u16string HTMLInputElementImp::getDirName()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setDirName(const std::u16string& dirName)
{
    // TODO: implement me!
}

bool HTMLInputElementImp::getDisabled()
{
    return getAttributeAsBoolean(u"disabled");
}

void HTMLInputElementImp::setDisabled(bool disabled)
{
    setAttributeAsBoolean(u"disabled", disabled);
}

html::HTMLFormElement HTMLInputElementImp::getForm()
{
    if (!form.expired())
        return form.lock();
    for (Element parent = getParentElement(); parent; parent = parent.getParentElement()) {
        if (html::HTMLFormElement::hasInstance(parent)) {
            form = std::dynamic_pointer_cast<HTMLFormElementImp>(parent.self());
            return form.lock();
        }
    }
    return nullptr;
}

file::FileList HTMLInputElementImp::getFiles()
{
    // TODO: implement me!
    return nullptr;
}

std::u16string HTMLInputElementImp::getFormAction()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setFormAction(const std::u16string& formAction)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getFormEnctype()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setFormEnctype(const std::u16string& formEnctype)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getFormMethod()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setFormMethod(const std::u16string& formMethod)
{
    // TODO: implement me!
}

bool HTMLInputElementImp::getFormNoValidate()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setFormNoValidate(bool formNoValidate)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getFormTarget()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setFormTarget(const std::u16string& formTarget)
{
    // TODO: implement me!
}

unsigned int HTMLInputElementImp::getHeight()
{
    // TODO: set defaultValue to the intrinsic length.
    return getAttributeAsUnsigned(u"height");
}

void HTMLInputElementImp::setHeight(unsigned int height)
{
    setAttributeAsUnsigned(u"height", height);
}

bool HTMLInputElementImp::getIndeterminate()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setIndeterminate(bool indeterminate)
{
    // TODO: implement me!
}

html::HTMLElement HTMLInputElementImp::getList()
{
    // TODO: implement me!
    return nullptr;
}

std::u16string HTMLInputElementImp::getMax()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setMax(const std::u16string& max)
{
    // TODO: implement me!
}

int HTMLInputElementImp::getMaxLength()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setMaxLength(int maxLength)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getMin()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setMin(const std::u16string& min)
{
    // TODO: implement me!
}

bool HTMLInputElementImp::getMultiple()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setMultiple(bool multiple)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getName()
{
    return getAttribute(u"name");
}

void HTMLInputElementImp::setName(const std::u16string& name)
{
    setAttribute(u"name", name);
}

std::u16string HTMLInputElementImp::getPattern()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setPattern(const std::u16string& pattern)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getPlaceholder()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setPlaceholder(const std::u16string& placeholder)
{
    // TODO: implement me!
}

bool HTMLInputElementImp::getReadOnly()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setReadOnly(bool readOnly)
{
    // TODO: implement me!
}

bool HTMLInputElementImp::getRequired()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setRequired(bool required)
{
    // TODO: implement me!
}

unsigned int HTMLInputElementImp::getSize()
{
    return getAttributeAsUnsigned(u"size", 20);
}

void HTMLInputElementImp::setSize(unsigned int size)
{
    setAttributeAsUnsigned(u"size", size);
}

std::u16string HTMLInputElementImp::getSrc()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setSrc(const std::u16string& src)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getStep()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setStep(const std::u16string& step)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getType()
{
    return typeKeywords[type];
}

void HTMLInputElementImp::setType(const std::u16string& type)
{
    setAttribute(u"type", type);
}

std::u16string HTMLInputElementImp::getDefaultValue()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setDefaultValue(const std::u16string& defaultValue)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getValue()
{
    return getAttribute(u"value");
}

void HTMLInputElementImp::setValue(const std::u16string& value)
{
    setAttribute(u"value", value);
}

Nullable<unsigned long long> HTMLInputElementImp::getValueAsDate()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setValueAsDate(Nullable<unsigned long long> valueAsDate)
{
    // TODO: implement me!
}

double HTMLInputElementImp::getValueAsNumber()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setValueAsNumber(double valueAsNumber)
{
    // TODO: implement me!
}

unsigned int HTMLInputElementImp::getWidth()
{
    // TODO: set defaultValue to the intrinsic length.
    return getAttributeAsUnsigned(u"width");
}

void HTMLInputElementImp::setWidth(unsigned int width)
{
    setAttributeAsUnsigned(u"width", width);
}

void HTMLInputElementImp::stepUp()
{
    // TODO: implement me!
}

void HTMLInputElementImp::stepUp(int n)
{
    // TODO: implement me!
}

void HTMLInputElementImp::stepDown()
{
    // TODO: implement me!
}

void HTMLInputElementImp::stepDown(int n)
{
    // TODO: implement me!
}

bool HTMLInputElementImp::getWillValidate()
{
    // TODO: implement me!
    return 0;
}

html::ValidityState HTMLInputElementImp::getValidity()
{
    // TODO: implement me!
    return nullptr;
}

std::u16string HTMLInputElementImp::getValidationMessage()
{
    // TODO: implement me!
    return u"";
}

bool HTMLInputElementImp::checkValidity()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setCustomValidity(const std::u16string& error)
{
    // TODO: implement me!
}

NodeList HTMLInputElementImp::getLabels()
{
    // TODO: implement me!
    return nullptr;
}

void HTMLInputElementImp::select()
{
    // TODO: implement me!
}

unsigned int HTMLInputElementImp::getSelectionStart()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setSelectionStart(unsigned int selectionStart)
{
    // TODO: implement me!
}

unsigned int HTMLInputElementImp::getSelectionEnd()
{
    // TODO: implement me!
    return 0;
}

void HTMLInputElementImp::setSelectionEnd(unsigned int selectionEnd)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getSelectionDirection()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setSelectionDirection(const std::u16string& selectionDirection)
{
    // TODO: implement me!
}

void HTMLInputElementImp::setSelectionRange(unsigned int start, unsigned int end)
{
    // TODO: implement me!
}

void HTMLInputElementImp::setSelectionRange(unsigned int start, unsigned int end, const std::u16string& direction)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getAlign()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setAlign(const std::u16string& align)
{
    // TODO: implement me!
}

std::u16string HTMLInputElementImp::getUseMap()
{
    // TODO: implement me!
    return u"";
}

void HTMLInputElementImp::setUseMap(const std::u16string& useMap)
{
    // TODO: implement me!
}

}
}
}
}
