/*
 * Copyright 2010-2015 Esrille Inc.
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

#ifndef ORG_W3C_DOM_BOOTSTRAP_RANGEIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_RANGEIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/ranges/Range.h>

#include <org/w3c/dom/DOMRectList.h>
#include <org/w3c/dom/DOMRect.h>
#include <org/w3c/dom/Node.h>
#include <org/w3c/dom/DocumentFragment.h>
#include <org/w3c/dom/ranges/Range.h>

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{

class DocumentImp;
class NodeImp;

typedef std::shared_ptr<NodeImp> NodePtr;
typedef std::shared_ptr<DocumentImp> DocumentPtr;

class RangeImp : public ObjectMixin<RangeImp>
{
    struct BoundaryPoint
    {
        NodePtr node;
        unsigned int offset;

        BoundaryPoint() :
            node(nullptr),
            offset(0)
        {}
        BoundaryPoint(const NodePtr& node, unsigned int offset = 0) :
            node(node),
            offset(offset)
        {}

        BoundaryPoint(const BoundaryPoint&) = default;
        BoundaryPoint(BoundaryPoint&&) = default;
        BoundaryPoint& operator=(const BoundaryPoint&) = default;
        BoundaryPoint& operator=(BoundaryPoint&&) = default;

        NodePtr getTargetNode() const;

        bool operator==(const BoundaryPoint& other) const {
            return node == other.node && offset == other.offset;
        }
        bool operator<(const BoundaryPoint& other) const;
    };

    std::weak_ptr<DocumentImp> ownerDocument;
    BoundaryPoint start;
    BoundaryPoint end;

public:
    RangeImp(const DocumentPtr& ownerDocument);

    void checkOwner();

    NodePtr getRoot();
    bool contains(const NodePtr& node);
    bool partiallyContains(const NodePtr& node);

    void onInsert(const NodePtr& parent, unsigned index, unsigned count);
    void onRemove(const NodePtr& parent, const NodePtr& node, unsigned index);
    void onReplaceData(const NodePtr& node, unsigned offset, unsigned count, unsigned length);
    void onSplitText(const NodePtr& parent, const NodePtr& node, const NodePtr& newNode, unsigned offset);
    void onNormalize(const NodePtr& currentNode, const NodePtr& node, unsigned length, unsigned index);

    // Range
    Node getStartContainer();
    unsigned int getStartOffset();
    Node getEndContainer();
    unsigned int getEndOffset();
    bool getCollapsed();
    Node getCommonAncestorContainer();
    void setStart(Node refNode, unsigned int offset);
    void setEnd(Node refNode, unsigned int offset);
    void setStartBefore(Node refNode);
    void setStartAfter(Node refNode);
    void setEndBefore(Node refNode);
    void setEndAfter(Node refNode);
    void collapse(bool toStart);
    void selectNode(Node refNode);
    void selectNodeContents(Node refNode);
    short compareBoundaryPoints(unsigned short how, ranges::Range sourceRange);
    void deleteContents();
    DocumentFragment extractContents();
    DocumentFragment cloneContents();
    void insertNode(Node node);
    void surroundContents(Node newParent);
    ranges::Range cloneRange();
    void detach();
    bool isPointInRange(Node node, unsigned int offset);
    short comparePoint(Node node, unsigned int offset);
    bool intersectsNode(Node node);
    std::u16string toString();
    // Range-52
    DocumentFragment createContextualFragment(const std::u16string& fragment);
    // Range-56
    DOMRectList getClientRects();
    DOMRect getBoundingClientRect();
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return ranges::Range::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return ranges::Range::getMetaData();
    }
};

typedef std::shared_ptr<RangeImp> RangePtr;

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_RANGEIMP_H_INCLUDED
