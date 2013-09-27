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

#include "CSSImportRuleImp.h"

#include <boost/bind.hpp>
#include <boost/version.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include "DocumentImp.h"
#include "WindowProxy.h"

#include "CSSInputStream.h"
#include "CSSParser.h"
#include "CSSStyleSheetImp.h"

#include "http/HTTPRequest.h"

#include "Test.util.h"

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{

using namespace css;

CSSImportRuleImp::CSSImportRuleImp(const std::u16string& href) :
    href(href),
    mediaList(std::make_shared<MediaListImp>()),
    request(0)
{
}

CSSImportRuleImp::~CSSImportRuleImp()
{
    delete request;
}

void CSSImportRuleImp::setMediaList(MediaListPtr other)
{
    mediaList = other;
}


// CSSRule
unsigned short CSSImportRuleImp::getType()
{
    return CSSRule::IMPORT_RULE;
}

std::u16string CSSImportRuleImp::getCssText()
{
    std::u16string text = u"@import url(" + href + u")";
    std::u16string media = mediaList.getMediaText();
    if (!media.empty())
        text += u" " + media;
    return text;
}

// CSSImportRule
std::u16string CSSImportRuleImp::getHref()
{
    return href;
}

stylesheets::MediaList CSSImportRuleImp::getMedia()
{
    return mediaList;
}

void CSSImportRuleImp::setMedia(const std::u16string& media)
{
    mediaList.setMediaText(media);
}

css::CSSStyleSheet CSSImportRuleImp::getStyleSheet()
{
    auto doc = document.lock();
    if (!doc)
        return nullptr;

    if (!styleSheet && !href.empty() && !request) {  // TODO: deal with ins. mem
        request = new(std::nothrow) HttpRequest(doc->getDocumentURI());
        if (request) {
            request->open(u"GET", href);
            request->setHandler(boost::bind(&CSSImportRuleImp::notify, this));
            doc->incrementLoadEventDelayCount();
            request->send();
        }
    }
    return styleSheet;
}

void CSSImportRuleImp::notify()
{
    auto doc = document.lock();
    if (!doc)
        return;

    if (request->getStatus() == 200) {
        boost::iostreams::stream<boost::iostreams::file_descriptor_source> stream(request->getContentDescriptor(), boost::iostreams::close_handle);
        CSSParser parser(request->getRequestMessage().getURL());
        CSSInputStream cssStream(stream, request->getResponseMessage().getContentCharset(), utfconv(doc->getCharacterSet()));
        styleSheet = parser.parse(doc, cssStream);
        if (auto imp = std::dynamic_pointer_cast<CSSStyleSheetImp>(styleSheet.self())) {
            imp->setParentStyleSheet(getParentStyleSheet());
        }
        if (4 <= getLogLevel())
            dumpStyleSheet(std::cerr, styleSheet.self());
    }

    if (WindowProxyPtr view = doc->getDefaultWindow())
        view->setViewFlags(Box::NEED_SELECTOR_REMATCHING);

    doc->decrementLoadEventDelayCount();
}

}
}
}
}
