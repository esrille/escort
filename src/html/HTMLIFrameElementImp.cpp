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

#include "HTMLIFrameElementImp.h"

#include <boost/bind.hpp>

#include "one_at_a_time.hpp"

constexpr auto Intern = &one_at_a_time::hash<char16_t>;

#include "DocumentImp.h"
#include "EventImp.h"
#include "HTMLUtil.h"
#include "WindowProxy.h"

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{

HTMLIFrameElementImp::HTMLIFrameElementImp(DocumentImp* ownerDocument) :
    ObjectMixin(ownerDocument, u"iframe"),
    blurListener(boost::bind(&HTMLIFrameElementImp::handleBlur, this, _1, _2)),
    loadInProgess(false)
{
    tabIndex = 0;
    addEventListener(u"blur", blurListener, false, EventTargetImp::UseDefault);
}

HTMLIFrameElementImp::HTMLIFrameElementImp(const HTMLIFrameElementImp& org) :
    ObjectMixin(org),
    window(org.window),
    blurListener(boost::bind(&HTMLIFrameElementImp::handleBlur, this, _1, _2)),
    loadInProgess(false)
{
    tabIndex = org.tabIndex;
    addEventListener(u"blur", blurListener, false, EventTargetImp::UseDefault);
}

HTMLIFrameElementImp::~HTMLIFrameElementImp()
{
}

void HTMLIFrameElementImp::open(const std::u16string& url, unsigned flags)
{
    auto owner = getOwnerDocumentImp();
    if (!owner)
        return;
    if (owner->getReadyState() != u"complete") {
        loadInProgess = true;
        owner->incrementLoadEventDelayCount();
    }
    if (auto proxy = owner->getDefaultWindow()->createWindowProxy(std::static_pointer_cast<HTMLIFrameElementImp>(self()), flags)) {
        window = proxy;
        proxy->open(url, u"_self");  // TODO: Check how location is created
    }  // TODO: else ...
}

void HTMLIFrameElementImp::notify(bool error)
{
    // The iframe load event steps
    auto event = std::make_shared<EventImp>();
    if (!error)
        event->initEvent(u"load", false, false);
    else
        event->initEvent(u"error", false, false);
    dispatchEvent(event);
    if (!loadInProgess)
        return;
    if (auto owner = getOwnerDocumentImp()) {
        owner->decrementLoadEventDelayCount();
        loadInProgess = false;
    }
}

void HTMLIFrameElementImp::handleMutation(events::MutationEvent mutation)
{
    std::u16string value = mutation.getNewValue();
    css::CSSStyleDeclaration style(getStyle());

    switch (Intern(mutation.getAttrName().c_str())) {
    case Intern(u"src"):
        window.open(getSrc(), u"_self");
        break;
    // Styles
    case Intern(u"height"):
        if (mapToDimension(value))
            style.setProperty(u"height", value, u"non-css");
        break;
    case Intern(u"hspace"):
        if (mapToDimension(value)) {
            style.setProperty(u"margin-left", value, u"non-css");
            style.setProperty(u"margin-right", value, u"non-css");
        }
        break;
    case Intern(u"vspace"):
        if (mapToDimension(value)) {
            style.setProperty(u"margin-top", value, u"non-css");
            style.setProperty(u"margin-bottom", value, u"non-css");
        }
        break;
    case Intern(u"width"):
        if (mapToDimension(value))
            style.setProperty(u"width", value, u"non-css");
        break;
    default:
        HTMLElementImp::handleMutation(mutation);
        break;
    }
}

void HTMLIFrameElementImp::handleBlur(EventListenerImp* listener, events::Event event)
{
    if (auto imp = std::dynamic_pointer_cast<DocumentImp>(window.getDocument().self()))
        imp->setFocus(0);
}

//
// HTMLIFrameElement
//

std::u16string HTMLIFrameElementImp::getSrc()
{
    return getAttributeAsURL(u"src");
}

void HTMLIFrameElementImp::setSrc(const std::u16string& src)
{
    setAttribute(u"src", src);
}

std::u16string HTMLIFrameElementImp::getSrcdoc()
{
    // TODO: implement me!
    return u"";
}

void HTMLIFrameElementImp::setSrcdoc(const std::u16string& srcdoc)
{
    // TODO: implement me!
}

std::u16string HTMLIFrameElementImp::getName()
{
    // TODO: implement me!
    return u"";
}

void HTMLIFrameElementImp::setName(const std::u16string& name)
{
    // TODO: implement me!
}

DOMSettableTokenList HTMLIFrameElementImp::getSandbox()
{
    // TODO: implement me!
    return nullptr;
}

void HTMLIFrameElementImp::setSandbox(const std::u16string& sandbox)
{
    // TODO: implement me!
}

bool HTMLIFrameElementImp::getSeamless()
{
    // TODO: implement me!
    return 0;
}

void HTMLIFrameElementImp::setSeamless(bool seamless)
{
    // TODO: implement me!
}

std::u16string HTMLIFrameElementImp::getWidth()
{
    return getAttribute(u"width");
}

void HTMLIFrameElementImp::setWidth(const std::u16string& width)
{
    // TODO: implement me!
}

std::u16string HTMLIFrameElementImp::getHeight()
{
    return getAttribute(u"height");
}

void HTMLIFrameElementImp::setHeight(const std::u16string& height)
{
    // TODO: implement me!
}

Document HTMLIFrameElementImp::getContentDocument()
{
    return window.getDocument();
}

html::Window HTMLIFrameElementImp::getContentWindow()
{
    return window;
}

std::u16string HTMLIFrameElementImp::getAlign()
{
    // TODO: implement me!
    return u"";
}

void HTMLIFrameElementImp::setAlign(const std::u16string& align)
{
    // TODO: implement me!
}

std::u16string HTMLIFrameElementImp::getFrameBorder()
{
    // TODO: implement me!
    return u"";
}

void HTMLIFrameElementImp::setFrameBorder(const std::u16string& frameBorder)
{
    // TODO: implement me!
}

std::u16string HTMLIFrameElementImp::getLongDesc()
{
    // TODO: implement me!
    return u"";
}

void HTMLIFrameElementImp::setLongDesc(const std::u16string& longDesc)
{
    // TODO: implement me!
}

std::u16string HTMLIFrameElementImp::getMarginHeight()
{
    // TODO: implement me!
    return u"";
}

void HTMLIFrameElementImp::setMarginHeight(const std::u16string& marginHeight)
{
    // TODO: implement me!
}

std::u16string HTMLIFrameElementImp::getMarginWidth()
{
    // TODO: implement me!
    return u"";
}

void HTMLIFrameElementImp::setMarginWidth(const std::u16string& marginWidth)
{
    // TODO: implement me!
}

std::u16string HTMLIFrameElementImp::getScrolling()
{
    // TODO: implement me!
    return u"";
}

void HTMLIFrameElementImp::setScrolling(const std::u16string& scrolling)
{
    // TODO: implement me!
}

}
}
}
}
