/*
 * Copyright 2012, 2013 Esrille Inc.
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

#ifndef HTML_REPLACED_ELEMENT_IMP_H
#define HTML_REPLACED_ELEMENT_IMP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "HTMLElementImp.h"

#include "http/HTTPRequest.h"
#include "css/BoxImage.h"

#include "Test.util.h"

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{

class HTMLReplacedElementImp : public HTMLElementImp
{
protected:
    HttpRequest* current;
    bool active;
    BoxImage* image;
    unsigned imageStart;

public:
    HTMLReplacedElementImp(DocumentImp* ownerDocument, const std::u16string& localName) :
        HTMLElementImp(ownerDocument, localName),
        current(0),
        active(true),
        image(0),
        imageStart(getTick())
    {
    }
    HTMLReplacedElementImp(const HTMLReplacedElementImp& org) :
        HTMLElementImp(org),
        current(0),     // TODO
        active(org.active),
        image(0),       // TODO
        imageStart(org.imageStart)
    {
    }
    ~HTMLReplacedElementImp()
    {
        delete current;
        delete image;
    }

    // Node - override
    virtual Node cloneNode(bool deep = true) {
        auto node = std::make_shared<HTMLReplacedElementImp>(*this);
        if (deep)
            node->cloneChildren(this);
        return node;
    }

    bool getIntrinsicSize(float& w, float& h) const {
        if (!active || !image)
            return false;
        w = image->getNaturalWidth();
        h = image->getNaturalHeight();
        return true;
    }
    BoxImage* getImage() const {
        return image;
    }
    unsigned getImageStart() const {
        return imageStart;
    }
    void setImageStart(unsigned tick) {
        imageStart = tick;
    }
};

}
}
}
}

#endif  // HTML_REPLACED_ELEMENT_IMP_H
