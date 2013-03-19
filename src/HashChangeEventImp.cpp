/*
 * Copyright 2013 Esrille Inc.
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

#include "HashChangeEventImp.h"

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{

HashChangeEventImp::HashChangeEventImp(const std::u16string& type, const std::u16string& oldURL, const std::u16string& newURL) :
    ObjectMixin(type),
    oldURL(oldURL),
    newURL(newURL)
{
}

std::u16string HashChangeEventImp::getOldURL()
{
    return oldURL;
}

std::u16string HashChangeEventImp::getNewURL()
{
    return newURL;
}

}
}
}
}
