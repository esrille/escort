// Generated by esidl 0.3.0.
// This file is expected to be modified for the Web IDL interface
// implementation.  Permission to use, copy, modify and distribute
// this file in any software license is hereby granted.

#ifndef ORG_W3C_DOM_BOOTSTRAP_EVENTSOURCEIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_EVENTSOURCEIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/eventsource/EventSource.h>
#include "EventTargetImp.h"

#include <org/w3c/dom/events/EventTarget.h>
#include <org/w3c/dom/eventsource/EventSource.h>
#include <org/w3c/dom/events/EventHandlerNonNull.h>

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{
class EventSourceImp : public ObjectMixin<EventSourceImp, EventTargetImp>
{
public:
    // EventSource
    std::u16string getUrl();
    unsigned short getReadyState();
    events::EventHandlerNonNull getOnopen();
    void setOnopen(events::EventHandlerNonNull onopen);
    events::EventHandlerNonNull getOnmessage();
    void setOnmessage(events::EventHandlerNonNull onmessage);
    events::EventHandlerNonNull getOnerror();
    void setOnerror(events::EventHandlerNonNull onerror);
    void close();
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return eventsource::EventSource::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return eventsource::EventSource::getMetaData();
    }
};

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_EVENTSOURCEIMP_H_INCLUDED
