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

#ifndef ES_CSSPARSER_H
#define ES_CSSPARSER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>
#include <deque>
#include <iostream>
#include <string>

#include <org/w3c/dom/css/CSSPrimitiveValue.h>
#include <org/w3c/dom/css/CSSStyleDeclaration.h>
#include <org/w3c/dom/css/CSSStyleSheet.h>

#include "CSSTokenizer.h"
#include "CSSSerialize.h"
#include "MediaListImp.h"
#include "url/URL.h"

extern int getLogLevel();

namespace org { namespace w3c { namespace dom { namespace bootstrap {

class CSSImportRuleImp;
class CSSMediaRuleImp;
class CSSParser;
class CSSRuleImp;
class CSSSelectorsGroup;
class CSSStyleDeclarationImp;
class CSSStyleRuleImp;
class CSSStyleSheetImp;
class DocumentImp;

typedef std::shared_ptr<CSSImportRuleImp> CSSImportRulePtr;
typedef std::shared_ptr<CSSMediaRuleImp> CSSMediaRulePtr;
typedef std::shared_ptr<CSSRuleImp> CSSRulePtr;
typedef std::shared_ptr<CSSStyleDeclarationImp> CSSStyleDeclarationPtr;
typedef std::shared_ptr<CSSStyleRuleImp> CSSStyleRulePtr;
typedef std::shared_ptr<CSSStyleSheetImp> CSSStyleSheetPtr;
typedef std::shared_ptr<DocumentImp> DocumentPtr;

struct CSSParserNumber
{
    double number;
    bool integer;   // true if number was expressed in an integer format.

    operator double() const {
        return number;
    }
    bool isInteger() const {
        return integer;
    }
    int toInteger() const {
        return static_cast<int>(number);
    }
};

struct CSSParserString
{
    const char16_t* text;
    ssize_t length;

    std::u16string toString(bool caseSensitive = true) const {
        const char16_t* end = text + length;
        std::u16string string;
        for (auto i = text; i < end; ++i) {
            char16_t c = *i;
            if (c != '\\') {
                if (!caseSensitive)
                    c = toLower(c);
                string += c;
                continue;
            }
            if (end <= ++i) {
                string += c;
                break;
            }
            c = *i;
            int x = isHexDigit(c);
            if (!x) {
                if (c != '\n') {  // TODO: check '\r' as well?
                    if (!caseSensitive)
                        c = toLower(c);
                    string += c;
                }
                continue;
            }
            // unescape
            char32_t code = c - x;
            int count;
            for (count = 1; count < 6; ++count) {
                if (end <= ++i)
                    break;
                c = *i;
                x = isHexDigit(c);
                if (x)
                    code = code * 16 + (c - x);
                else if (isSpace(c))
                    break;
                else {
                    --i;
                    break;
                }
            }
            if (count == 6 && ++i < end) {
                c = *i;
                if (!isSpace(c))
                    --i;
            }
            if (code) {
                char16_t s[2];
                if (char16_t* t = utf32to16(code, s)) {
                    for (char16_t* p = s; p < t; ++p)
                        string += caseSensitive ? *p : toLower(*p);
                }
            } else {
                while (0 < count--)
                    string += u'0';
                if (isSpace(c))
                    string += c;
            }
        }
        return string;
    }

    operator std::u16string() const {
        return toString(false);
    }

    // returns 0x00FFFFFF upon an invalid #hex color
    unsigned toRGB() const {
        unsigned rgb = 0;
        unsigned h;
        if (length == 3) {
            for (int i = 0; i < 3; ++i) {
                if (!(h = isHexDigit(text[i])))
                    return 0x00FFFFFF;
                h = text[i] - h;
                rgb <<= 4;
                rgb += h;
                rgb <<= 4;
                rgb += h;
            }
        } else if (length == 6) {
            for (int i = 0; i < 6; ++i) {
                if (!(h = isHexDigit(text[i])))
                    return 0x00FFFFFF;
                rgb <<= 4;
                rgb += text[i] - h;
            }
        } else
            return 0x00FFFFFF;
        return rgb | 0xFF000000u;
    }
    bool operator==(const char16_t* s) const {
        return std::memcmp(text, s, length * sizeof(char16_t)) == 0 && s[length] == 0;
    }
    bool compareIgnoreCase(const char16_t* s, ssize_t len) const {
        if (length != len)
            return false;
        for (const char16_t* t = text; 0 < len; --len) {
            if (toLower(*s++) != toLower(*t++))
                return false;
        }
        return true;
    }

    void clear() {
        length = 0;
    }
};

struct CSSParserExpr;

struct CSSParserTerm
{
    static const unsigned short CSS_TERM_FUNCTION = 201;
    static const unsigned short CSS_TERM_OPERATOR = 202;
    static const unsigned short CSS_TERM_BAR_NTH = 203;
    static const unsigned short CSS_TERM_INDEX = 204;
    static const unsigned short CSS_TERM_END = 205;

    // Note CSSParserTerm must begin with op, and then unit as they are initialized
    // via the initialization list.
    short op;  // '/', ',', or '\0'
    unsigned short unit;
    CSSParserNumber number;  // TODO: number should be float
    unsigned rgb;   // also used as the keyword index
    CSSParserString text;
    CSSParserExpr* expr;  // for function

    unsigned propertyID;

    // Internal fields
    CSSParser* parser;

    std::u16string getCssText();
    int getIndex() const {
        if (unit == CSS_TERM_INDEX)
            return rgb;
        else
            return -1;
    }
    double getNumber() const {
        return number;
    }
    std::u16string getString(bool caseSensitive = true) const {
        switch (unit) {
        case css::CSSPrimitiveValue::CSS_STRING:
        case css::CSSPrimitiveValue::CSS_URI:
        case css::CSSPrimitiveValue::CSS_IDENT:
        case css::CSSPrimitiveValue::CSS_UNICODE_RANGE:
        case CSS_TERM_FUNCTION:
            return text.toString(caseSensitive);
        default:
            return u"";
        }
    }
    std::u16string getURL() const;
};

struct CSSParserExpr
{
    std::deque<CSSParserTerm> list;
    std::u16string priority;
    void push_front(const CSSParserTerm& term) {
        list.push_front(term);
    }
    void push_back(const CSSParserTerm& term) {
        list.push_back(term);
    }
    void setPriority(const std::u16string& priority) {
        this->priority = priority;
    }
    const std::u16string& getPriority() const {
        return priority;
    }
    std::u16string getCssText();
    bool isInherit() const {
        if (list.size() != 1)
            return false;
        const CSSParserTerm& term = list.front();
        if (term.unit == css::CSSPrimitiveValue::CSS_IDENT && term.text == u"inherit")
            return true;
        return false;
    }
};

class CSSParser
{
    URL baseURL;
    DocumentPtr document;
    CSSTokenizer tokenizer;
    CSSStyleSheetPtr styleSheet;
    CSSStyleDeclarationPtr styleDeclaration;
    CSSParserExpr* styleExpression;
    CSSSelectorsGroup* selectorsGroup;
    CSSMediaRulePtr mediaRule;
    bool caseSensitive;  // for element names and attribute names.
    bool importable;

    MediaListPtr mediaList;
    CSSRulePtr rule;

    void reset(const std::u16string cssText) {
        tokenizer.reset(cssText);
    }
public:
    CSSParser(const std::u16string base = u"") :
        baseURL(base),
        styleSheet(0),
        styleExpression(0),
        selectorsGroup(0),
        mediaRule(0),
        caseSensitive(false),
        importable(true)
    {
    }

    std::u16string getURL(const std::u16string& href) const {
        if (!baseURL.isEmpty()) {
            URL url(baseURL, href);
            return url;
        }
        return href;
    }

    css::CSSStyleSheet parse(const DocumentPtr& document, const std::u16string& cssText);
    css::CSSStyleDeclaration parseDeclarations(const std::u16string& cssDecl);
    CSSParserExpr* parseExpression(const std::u16string& cssExpr);
    MediaListPtr parseMediaList(const std::u16string& mediaText);
    CSSSelectorsGroup* parseSelectorsGroup(const std::u16string& selectors);

    DocumentPtr getDocument() const {
        return document;
    }

    CSSTokenizer* getTokenizer() {
        return &tokenizer;
    }
    CSSStyleSheetPtr getStyleSheet() {
        return styleSheet;
    }
    void setStyleDeclaration(CSSStyleDeclarationPtr styleDeclaration) {
        this->styleDeclaration = styleDeclaration;
    }
    CSSStyleDeclarationPtr getStyleDeclaration() {
        return styleDeclaration;
    }

    void setExpression(CSSParserExpr* styleExpression) {
        this->styleExpression = styleExpression;
    }
    CSSParserExpr* getExpression() {
        return styleExpression;
    }

    void setSelectorsGroup(CSSSelectorsGroup* selectorsGroup) {
        this->selectorsGroup = selectorsGroup;
    }
    CSSSelectorsGroup* getSelectorsGroup() {
        return selectorsGroup;
    }

    void setMediaRule(CSSMediaRulePtr mediaRule) {
        this->mediaRule = mediaRule;
    }
    CSSMediaRulePtr getMediaRule() {
        return mediaRule;
    }

    void setMediaList(MediaListPtr mediaList) {
        this->mediaList = mediaList;
    }
    MediaListPtr getMediaList() {
        if (!mediaList)
            mediaList = std::make_shared<MediaListImp>();
        return mediaList;
    }

    void setRule(CSSRulePtr rule) {
        this->rule = rule;
    }
    CSSRulePtr getRule() {
        return rule;
    }

    bool getCaseSensitivity() const {
        return caseSensitive;
    }
    void setCaseSensitivity(bool value) {
        caseSensitive = value;
    }

    void disableImport() {
        importable = false;
    }
    bool isImportable() {
        return importable;
    }
};

inline void CSSerror(CSSParser* parser, const char* message, ...)
{
    if (3 <= getLogLevel())
        std::cerr << message << '\n';
}

inline int CSSlex(CSSParser* parser)
{
    return parser->getTokenizer()->getToken();
}

}}}}  // org::w3c::dom::bootstrap

int CSSparse(org::w3c::dom::bootstrap::CSSParser* parser);

#endif  // ES_CSSPARSER_H
