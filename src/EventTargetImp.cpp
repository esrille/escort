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

#include "EventTargetImp.h"

#include <org/w3c/dom/events/Event.h>

#include "DocumentImp.h"
#include "WindowImp.h"
#include "EventImp.h"
#include "NodeImp.h"
#include "UIEventImp.h"
#include "WindowProxy.h"
#include "html/HTMLTemplateElementImp.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

void EventTargetImp::invoke(const EventPtr& event)
{
    auto found = map.find(event->getType());
    if (found == map.end())
        return;
    std::list<Listener>& listeners = found->second;
    for (auto i = listeners.begin(); i != listeners.end(); ++i) {
        if (event->getStopImmediatePropagationFlag())
            return;
        switch (event->getEventPhase()) {
        case events::Event::CAPTURING_PHASE:
            if (i->useCapture() && !i->useDefault())
                i->listener.handleEvent(event);
            break;
        case events::Event::BUBBLING_PHASE:
            if (!i->useCapture() && !i->useDefault())
                i->listener.handleEvent(event);
            break;
        case events::Event::AT_TARGET:
            if (!i->useDefault())
                i->listener.handleEvent(event);
            break;
        case EventImp::DEFAULT_PHASE:
            if (i->useDefault())
                i->listener.handleEvent(event);
        default:
            break;
        }
    }
}

EventListenerPtr EventTargetImp::getEventHandlerListener(const std::u16string& type)
{
    auto found = map.find(type);
    if (found == map.end())
        return 0;
    std::list<Listener>& listeners = found->second;
    for (auto i = listeners.begin(); i != listeners.end(); ++i) {
        if (i->useEventHandler())
            return std::dynamic_pointer_cast<EventListenerImp>(i->listener.self());
    }
    return 0;
}

Object EventTargetImp::getEventHandler(const std::u16string& type)
{
    if (auto listener = getEventHandlerListener(type))
        return listener->getEventHandler();
    return nullptr;
}

// EventTarget
void EventTargetImp::addEventListener(const std::u16string&  type, events::EventListener listener, bool useCapture, unsigned flags)
{
    if (!listener)
        return;
    if (useCapture)
        flags |= UseCapture;
    Listener item{ listener, flags };

    auto found = map.find(type);
    if (found == map.end()) {
        std::list<Listener> listeners;
        listeners.push_back(item);
        map.insert(std::pair<std::u16string, std::list<Listener>>(type, listeners));
        return;
    }

    std::list<Listener>& listeners = found->second;
    for (auto i = listeners.begin(); i != listeners.end(); ++i) {
        if (*i == item)
            return;
    }
    listeners.push_back(item);
}

void EventTargetImp::removeEventListener(const std::u16string&  type, events::EventListener listener, bool useCapture, unsigned flags)
{
    if (!listener)
        return;
    if (useCapture)
        flags |= UseCapture;
    Listener item{ listener, flags };

    auto found = map.find(type);
    if (found == map.end())
        return;

    std::list<Listener>& listeners = found->second;
    for (auto i = listeners.begin(); i != listeners.end(); ++i) {
        if (*i == item) {
            listeners.erase(i);
            return;
        }
    }
}

// TODO: Refine using virtual!
bool EventTargetImp::dispatchEvent(events::Event evt)
{
    auto event = std::dynamic_pointer_cast<EventImp>(evt.self());
    if (!event)
        return false;

    event->setDispatchFlag(true);
    if (auto window = std::dynamic_pointer_cast<WindowImp>(self()))
        event->setTarget(window->getWindowProxy());
    else
        event->setTarget(self());

    if (auto node = std::dynamic_pointer_cast<NodeImp>(self())) {
        DocumentPtr document = std::dynamic_pointer_cast<DocumentImp>(node);
        if (!document)
            document = node->getOwnerDocumentImp();
        assert(document);

        document->enter();

        std::list<EventTargetPtr> eventPath;
        for (NodePtr ancestor = node->getParent(); ancestor; ancestor = ancestor->getParent()) {
            if (auto shadowTree = std::dynamic_pointer_cast<HTMLTemplateElementImp>(ancestor)) {
                if (auto host = std::dynamic_pointer_cast<NodeImp>(shadowTree->getHost().self())) {
                    // TODO: Fix 'target' of the event as well.
                    // TODO: Check the mouseover and mouseout events.
                    ancestor = host;
                    if (!std::dynamic_pointer_cast<UIEventImp>(event)) {
                        // To repaint the window, we still need to notify the bound document
                        // of the event.
                        // TODO: Support nesting of bound elements.
                        eventPath.push_front(ancestor->getOwnerDocumentImp());
                        break;
                    }
                }
            }
            eventPath.push_front(ancestor);
        }

        // cf. http://www.whatwg.org/specs/web-apps/current-work/multipage/webappapis.html#events-and-the-window-object
        if (document && event->getType() != u"load") {
            if (WindowProxyPtr view = document->getDefaultWindow())
                eventPath.push_front(view->getWindowPtr());
        }

        event->setEventPhase(events::Event::CAPTURING_PHASE);
        for (auto i = eventPath.begin(); i != eventPath.end(); ++i) {
            if (event->getStopPropagationFlag())
                break;
            event->setCurrentTarget(*i);
            (*i)->invoke(event);
        }

        event->setEventPhase(events::Event::AT_TARGET);
        if (!event->getStopPropagationFlag()) {
            event->setCurrentTarget(node);
            node->invoke(event);
        }

        if (event->getBubbles()) {
            event->setEventPhase(events::Event::BUBBLING_PHASE);
            for (auto i = eventPath.rbegin(); i != eventPath.rend(); ++i) {
                if (event->getStopPropagationFlag())
                    break;
                event->setCurrentTarget(*i);
                (*i)->invoke(event);
            }
        }

        if (!event->getDefaultPrevented()) {
            // TODO: Call default actions;
            // cf. http://www.w3.org/TR/DOM-Level-3-Events/#event-flow-default-cancel
            // cf. http://www.w3.org/TR/xbl/#the-default-phase0
            event->setEventPhase(EventImp::DEFAULT_PHASE);
            event->setCurrentTarget(node);
            node->invoke(event);
            if (event->getBubbles()) {
                for (auto i = eventPath.rbegin(); i != eventPath.rend(); ++i) {
                    if (event->getDefaultPrevented())
                        break;
                    event->setCurrentTarget(*i);
                    (*i)->invoke(event);
                }
            }
        }

        document->exit();

    } else if (auto window = std::dynamic_pointer_cast<WindowImp>(self())) {
        if (auto proxy = std::dynamic_pointer_cast<WindowProxy>(window->getWindowProxy()->self())) {
            window->enter(proxy.get());
            event->setEventPhase(events::Event::AT_TARGET);
            event->setCurrentTarget(self());
            invoke(event);
            window->exit(proxy.get());
        }
    }
    event->setDispatchFlag(false);
    event->setEventPhase(events::Event::AT_TARGET);
    event->setCurrentTarget(nullptr);

    return !event->getDefaultPrevented();
}

EventTargetImp::EventTargetImp() :
    ObjectMixin()
{
}

EventTargetImp::EventTargetImp(const EventTargetImp& other) :
    ObjectMixin(other)
{
    // TODO: Check what needs to be copied.
}

}}}}  // org::w3c::dom::bootstrap
