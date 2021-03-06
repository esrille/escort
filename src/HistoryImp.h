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

#ifndef ORG_W3C_DOM_BOOTSTRAP_HISTORYIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_HISTORYIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/html/History.h>

#include <deque>

#include "WindowImp.h"
#include "url/URL.h"

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{

class WindowProxy;

class HistoryImp : public ObjectMixin<HistoryImp>
{
    struct SessionHistoryEntry
    {
        URL url;
        WindowPtr window;

        SessionHistoryEntry(const URL& url, const WindowPtr& window);
        SessionHistoryEntry(const WindowPtr& window);
        SessionHistoryEntry(const SessionHistoryEntry& other);
    };

    WindowProxy* window;
    std::deque<SessionHistoryEntry> sessionHistory;
    int currentSession;
    bool replace;

    void removeAfterCurrent();

public:
    HistoryImp(WindowProxy* window) :
        window(window),
        currentSession(0),
        replace(false)
    {
    }

    void setReplace(bool value) {
        replace = value;
    }

    // Update the session history with the new page
    void update(const URL& url, const WindowPtr& window);
    void update(const WindowPtr& window);

    // History
    int getLength();
    Any getState();
    void go();
    void go(int delta);
    void back();
    void forward();
    void pushState(Any data, const std::u16string& title);
    void pushState(Any data, const std::u16string& title, const std::u16string& url);
    void replaceState(Any data, const std::u16string& title);
    void replaceState(Any data, const std::u16string& title, const std::u16string& url);
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return html::History::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return html::History::getMetaData();
    }
};

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_HISTORYIMP_H_INCLUDED
