/*------------------------------------------------------------------------------

    Copyright (c) 2004 Media Development Loan Fund
 
    This file is part of the LiveSupport project.
    http://livesupport.campware.org/
    To report bugs, send an e-mail to bugs@campware.org
 
    LiveSupport is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
  
    LiveSupport is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with LiveSupport; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 
 
    Author   : $Author: fgerlits $
    Version  : $Revision: 1.19 $
    Location : $Source: /home/paul/cvs2svn-livesupport/newcvsrepo/livesupport/modules/core/src/AudioClip.cxx,v $

------------------------------------------------------------------------------*/

/* ============================================================ include files */

#ifdef HAVE_CONFIG_H
#include "configure.h"
#endif

#include <sstream>
#include <typeinfo>
#include <fileref.h>    // for TagLib
#include <mpegfile.h>   // for TagLib
#include <id3v1tag.h>   // for TagLib
#include <id3v2tag.h>   // for TagLib

#include "LiveSupport/Core/TagConversion.h"
#include "LiveSupport/Core/AudioClip.h"

using namespace boost::posix_time;

using namespace LiveSupport::Core;

/* ===================================================  local data structures */


/* ================================================  local constants & macros */

/*------------------------------------------------------------------------------
 *  The name of the config element for this class
 *----------------------------------------------------------------------------*/
const std::string AudioClip::configElementNameStr = "audioClip";

/**
 *  The name of the attribute to get the id of the audio clip.
 */
static const std::string    idAttrName = "id";

/**
 *  The name of the attribute to get the URI of the audio clip.
 */
static const std::string    uriAttrName = "uri";

/**
 *  The name of the attribute to get the playlength of the audio clip.
 */
static const std::string    playlengthAttrName = "playlength";

/**
 *  The name of the attribute to get the title of the audio clip.
 */
static const std::string    titleAttrName = "title";

/**
 *  The name of the metadata child element.
 */
static const std::string    metadataElementName = "metadata";

/**
 *  The prefix of the extent (length) metadata element.
 */
static const std::string    extentElementPrefix = "dcterms";

/**
 *  The name of the extent (length) metadata element.
 */
static const std::string    extentElementName = "extent";

/**
 *  The URI identifier for the "dcterms" prefix
 */
static const std::string    extentElementUri = "http://purl.org/dc/terms/";

/**
 *  The prefix of the title metadata element.
 */
static const std::string    titleElementPrefix = "dc";

/**
 *  The name of the title metadata element.
 */
static const std::string    titleElementName = "title";

/**
 *  The URI identifier for the "dc" prefix
 */
static const std::string    titleElementUri ="http://purl.org/dc/elements/1.1/";

/**
 *  The URI identifier for the default XML namespace
 */
static const std::string    defaultPrefixUri ="http://www.streamonthefly.org/";


/* ===============================================  local function prototypes */


/* =============================================================  module code */

/*------------------------------------------------------------------------------
 *  Copy constructor.
 *----------------------------------------------------------------------------*/
AudioClip :: AudioClip(const AudioClip & otherAudioClip)   throw ()
                        : Playable(AudioClipType)
{
    this->id            = otherAudioClip.id;
    this->title         = otherAudioClip.title;
    this->playlength    = otherAudioClip.playlength;
    this->uri           = otherAudioClip.uri;
    this->token         = otherAudioClip.token;

    if (otherAudioClip.xmlAudioClip) {
        xmlAudioClip.reset(new xmlpp::Document);
        xmlAudioClip->create_root_node_by_import(
                        otherAudioClip.xmlAudioClip->get_root_node(),
                        true);     // true == recursive
    }
}


/*------------------------------------------------------------------------------
 *  Test constructor without title.
 *----------------------------------------------------------------------------*/
AudioClip :: AudioClip(Ptr<UniqueId>::Ref       id,
                  Ptr<time_duration>::Ref       playlength,
                  Ptr<const std::string>::Ref   uri)
                                                           throw ()
                        : Playable(AudioClipType)
{
    this->id         = id;
    this->title.reset(new Glib::ustring(""));
    this->playlength = playlength;
    this->uri        = uri;

    setMetadata(title, titleElementName, titleElementPrefix);
    
    Ptr<const Glib::ustring>::Ref playlengthString(new const Glib::ustring(
                                        to_simple_string(*playlength) ));
    setMetadata(playlengthString, extentElementName, extentElementPrefix);
}


/*------------------------------------------------------------------------------
 *  Test constructor with title.
 *----------------------------------------------------------------------------*/
AudioClip :: AudioClip(Ptr<UniqueId>::Ref               id,
                       Ptr<const Glib::ustring>::Ref    title,
                       Ptr<time_duration>::Ref          playlength,
                       Ptr<const std::string>::Ref      uri)
                                                           throw ()
                        : Playable(AudioClipType)
{
    this->id         = id;
    this->title      = title;
    this->playlength = playlength;
    this->uri        = uri;

    setMetadata(title, titleElementName, titleElementPrefix);

    Ptr<const Glib::ustring>::Ref playlengthString(new const Glib::ustring(
                                        to_simple_string(*playlength) ));
    setMetadata(playlengthString, extentElementName, extentElementPrefix);
}


/*------------------------------------------------------------------------------ *  Constructor without ID.
 *----------------------------------------------------------------------------*/AudioClip :: AudioClip(Ptr<const Glib::ustring>::Ref    title,
                       Ptr<time_duration>::Ref          playlength,
                       Ptr<const std::string>::Ref      uri)
                                                           throw ()
                        : Playable(AudioClipType)
{
    this->title      = title;
    this->playlength = playlength;
    this->uri        = uri;

    setMetadata(title, titleElementName, titleElementPrefix);
 
    Ptr<const Glib::ustring>::Ref playlengthString(new const Glib::ustring(
                                        to_simple_string(*playlength) ));
    setMetadata(playlengthString, extentElementName, extentElementPrefix);
}
 
 
/*------------------------------------------------------------------------------
 *  Set the value of the title field.
 *----------------------------------------------------------------------------*/
void
AudioClip :: setTitle(Ptr<const Glib::ustring>::Ref title)
                                                throw ()
{
    this->title = title;
    setMetadata(title, titleElementName, titleElementPrefix);
}


/*------------------------------------------------------------------------------
 *  Create an audio clip object based on an XML element.
 *----------------------------------------------------------------------------*/
void
AudioClip :: configure(const xmlpp::Element  & element)
                                               throw (std::invalid_argument)
{
    if (element.get_name() != configElementNameStr) {
        std::string eMsg = "bad configuration element ";
        eMsg += element.get_name();
        throw std::invalid_argument(eMsg);
    }
    
    const xmlpp::Attribute    * attribute;

    if (!id) {
        if (!(attribute = element.get_attribute(idAttrName))) {
            std::string eMsg = "missing attribute ";
            eMsg += idAttrName;
            throw std::invalid_argument(eMsg);
        }
        id.reset(new UniqueId(attribute->get_value()));
    }

    if (!playlength
            && (attribute = element.get_attribute(playlengthAttrName))) {
        playlength.reset(new time_duration(duration_from_string(
                                                     attribute->get_value() )));
        Ptr<const Glib::ustring>::Ref playlengthString(new const Glib::ustring(
                                                     attribute->get_value() ));
        setMetadata(playlengthString, extentElementName, extentElementPrefix);
    }

    if (!title
            && (attribute = element.get_attribute(titleAttrName))) {
        title.reset(new const Glib::ustring(attribute->get_value()));
        setMetadata(title, titleElementName, titleElementPrefix);
    }

    if (!uri 
            && (attribute = element.get_attribute(uriAttrName))) {
        uri.reset(new const std::string(attribute->get_value()));
    }

    xmlpp::Node::NodeList       childNodes 
                                = element.get_children(metadataElementName);
    xmlpp::Node::NodeList::iterator it = childNodes.begin();

    if (it != childNodes.end()) {
        const xmlpp::Element    * metadataElement 
                                = dynamic_cast<const xmlpp::Element*> (*it);

        xmlAudioClip.reset(new xmlpp::Document);
        xmlpp::Element*     root = xmlAudioClip->create_root_node("audioClip");
        root->set_attribute("id", std::string(*id));
        root->import_node(metadataElement, true);    // true = recursive

        const xmlpp::Node::NodeList dataFieldList
                                    = metadataElement->get_children();
        xmlpp::Node::NodeList::const_iterator listIt = dataFieldList.begin();

        while (listIt != dataFieldList.end()) {
            const xmlpp::Node*  dataNode = *listIt;
            std::string         prefix   = dataNode->get_namespace_prefix();
            std::string         name     = dataNode->get_name();
            const xmlpp::Element*
                                dataElement 
                              = dynamic_cast<const xmlpp::Element*> (dataNode);
            if (!dataElement) {
                ++listIt;
                continue;
            }

            if (!playlength && prefix  == extentElementPrefix
                            && name    == extentElementName) {
                if (dataElement->has_child_text()) {
                    playlength.reset(new time_duration(duration_from_string(
                            dataElement->get_child_text()->get_content() )));
                } else {              // or just leave blank?  bad either way
                    playlength.reset(new time_duration(0,0,0,0));
                }
            }

            if (!title && prefix  == titleElementPrefix
                       && name    == titleElementName) {
                Glib::ustring       value;
                if (dataElement->has_child_text()) {
                    value = dataElement->get_child_text()->get_content();
                } else {
                    value = "";
                }
                Ptr<const Glib::ustring>::Ref ptrToValue(
                                                new const Glib::ustring(value));
                title = ptrToValue;
            }

            ++listIt;
        }
        
        ++it;
        if (it != childNodes.end()) {
            std::string eMsg = "more than one ";
            eMsg += metadataElementName;
            eMsg += " XML element";
            throw std::invalid_argument(eMsg);
        }
    }

    if (!playlength) {
        std::string eMsg = "missing attribute ";
        eMsg += playlengthAttrName;
        eMsg += " or metadata element ";
        eMsg += extentElementPrefix + ":" + extentElementName;
        throw std::invalid_argument(eMsg);
    }
    
    if (!title) {
        std::string eMsg = "missing attribute ";
        eMsg += titleAttrName;
        eMsg += " or metadata element ";
        eMsg += titleElementPrefix + ":" + titleElementName;
        throw std::invalid_argument(eMsg);
    }
}


/*------------------------------------------------------------------------------
 *  Return the value of a metadata field.
 *----------------------------------------------------------------------------*/
Ptr<Glib::ustring>::Ref
AudioClip :: getMetadata(const string &key) const
                                                throw ()
{
    std::string name, prefix;
    separateNameAndNameSpace(key, name, prefix);

    Ptr<Glib::ustring>::Ref value;

    if (! xmlAudioClip) {
        return value;
    }
    xmlpp::Element*         rootNode = xmlAudioClip->get_root_node();
    if (! rootNode) {
        return value;
    }
    xmlpp::Node::NodeList   rootList = rootNode->get_children("metadata");
    if (rootList.size() == 0) {
        return value;
    }
    
    xmlpp::Node*            metadata = rootList.front();
    xmlpp::Node::NodeList   nodeList = metadata->get_children(name);
    xmlpp::Node::NodeList::iterator it = nodeList.begin();
    
    while (it != nodeList.end()) {
        xmlpp::Node*        node = *it;
        if (node->get_namespace_prefix() == prefix) {
            xmlpp::Element* element = dynamic_cast<xmlpp::Element*> (node);
            value.reset(new Glib::ustring(element->get_child_text()
                                                 ->get_content()));
            return value;
        }
        ++it;
    }

    return value;
}


/*------------------------------------------------------------------------------
 *  Set the value of a metadata field (public).
 *----------------------------------------------------------------------------*/
void
AudioClip :: setMetadata(Ptr<const Glib::ustring>::Ref value, 
                         const std::string &key)
                                                throw ()
{
    std::string name, prefix;
    separateNameAndNameSpace(key, name, prefix);
    setMetadata(value, name, prefix);
}


/*------------------------------------------------------------------------------
 *  Set the value of a metadata field (private).
 *----------------------------------------------------------------------------*/
void
AudioClip :: setMetadata(Ptr<const Glib::ustring>::Ref value, 
                         const std::string &name, const std::string &prefix)
                                                throw ()
{
    if (prefix == extentElementPrefix && name == extentElementName) {
        playlength.reset(new time_duration(
                                duration_from_string(*value) ));
    }
    
    if (prefix == titleElementPrefix && name == titleElementName) {
        title = value;
    }

    if (! xmlAudioClip) {
        xmlAudioClip.reset(new xmlpp::Document);
    }
    xmlpp::Element*         rootNode = xmlAudioClip->get_root_node();
    if (! rootNode) {
        rootNode = xmlAudioClip->create_root_node("audioClip");
    }
    xmlpp::Node::NodeList   rootList = rootNode->get_children("metadata");
    xmlpp::Element*         metadata;
    if (rootList.size() > 0) {
        metadata = dynamic_cast<xmlpp::Element*> (rootList.front());
    } else {
        metadata = rootNode->add_child("metadata");
        metadata->set_namespace_declaration(defaultPrefixUri);
        metadata->set_namespace_declaration(titleElementUri, 
                                            titleElementPrefix);
        metadata->set_namespace_declaration(extentElementUri, 
                                            extentElementPrefix);
    }

    xmlpp::Node::NodeList   nodeList    = metadata->get_children(name);
    xmlpp::Node::NodeList::iterator it  = nodeList.begin();
    xmlpp::Element*         element     = 0;

    while (it != nodeList.end()) {
        xmlpp::Node*        node = *it;
        if (node->get_namespace_prefix() == prefix) {
            element = dynamic_cast<xmlpp::Element*> (nodeList.front());
            break;
        }
        ++it;
    }
    
    if (it == nodeList.end()) {
        element = metadata->add_child(name);
        try {
            element->set_namespace(prefix);
        }
        catch (xmlpp::exception &e) {
        // this namespace has not been declared; well OK, do nothing then
        }
    }
    
    element->set_child_text(*value);
}


/*------------------------------------------------------------------------------
 *  Return a string containing the essential fields of this object, in XML.
 *----------------------------------------------------------------------------*/
Ptr<Glib::ustring>::Ref
AudioClip :: getXmlString(void)                 throw ()
{
    Ptr<Glib::ustring>::Ref     xmlString(new Glib::ustring);
    
    xmlString->append("<");
    xmlString->append(configElementNameStr + " ");
    xmlString->append(idAttrName + "=\"" 
                                 + std::string(*id) 
                                 + "\" ");
    xmlString->append(playlengthAttrName + "=\"" 
                                         + to_simple_string(*playlength)
                                         + "\" ");
    xmlString->append(Glib::ustring(titleAttrName) + "=\"" 
                                                   + *title
                                                   + "\"/>");
    return xmlString;
}


/*------------------------------------------------------------------------------
 *  Return a string containing the metadata of the audio clip, in XML.
 *----------------------------------------------------------------------------*/
Ptr<Glib::ustring>::Ref
AudioClip :: getMetadataString()                throw ()
{
    Ptr<Glib::ustring>::Ref metadataString;

    if (!xmlAudioClip) {
        return metadataString;
    }
    
    metadataString.reset(new Glib::ustring(xmlAudioClip->write_to_string() ));

    return metadataString;
}


/*------------------------------------------------------------------------------
 *  Read the metadata contained in the id3v2 tag of the binary sound file.
 *----------------------------------------------------------------------------*/
void
AudioClip :: readTag()                          throw (std::invalid_argument)
{
    if (!TagConversion::isConfigured()) {
        throw std::invalid_argument("tag conversion table not loaded");
    }

    if (!getUri()) {
        throw std::invalid_argument("audio clip has no uri field");
    }
    
    if (!TagLib::File::isReadable(getUri()->c_str())) {
        throw std::invalid_argument("binary sound file not found");
    }
    
    TagLib::FileRef         genericFileRef(getUri()->c_str());
    TagLib::Tag*            tag = genericFileRef.tag();
    if (!tag) {
        return;
    }

    Ptr<const Glib::ustring>::Ref   value;                  // true = unicode
    if (TagConversion::existsId3Tag("Artist")) {
        value.reset(new const Glib::ustring(tag->artist().to8Bit(true)));
        setMetadata(value, TagConversion::id3ToDublinCore("Artist"));
    }
    
    if (TagConversion::existsId3Tag("Title")) {
        value.reset(new const Glib::ustring(tag->title().to8Bit(true)));
        setMetadata(value, TagConversion::id3ToDublinCore("Title"));
    }
    
    if (TagConversion::existsId3Tag("Album")) {
        value.reset(new const Glib::ustring(tag->album().to8Bit(true)));
        setMetadata(value, TagConversion::id3ToDublinCore("Album"));
    }
    
    if (TagConversion::existsId3Tag("Comment")) {
        value.reset(new const Glib::ustring(tag->comment().to8Bit(true)));
        setMetadata(value, TagConversion::id3ToDublinCore("Comment"));
    }
    
    if (TagConversion::existsId3Tag("Genre")) {
        value.reset(new const Glib::ustring(tag->genre().to8Bit(true)));
        setMetadata(value, TagConversion::id3ToDublinCore("Genre"));
    }
    
    if (TagConversion::existsId3Tag("Year")) {
        std::stringstream   yearString;
        yearString << tag->year();
        value.reset(new const Glib::ustring(yearString.str()));
        setMetadata(value, TagConversion::id3ToDublinCore("Year"));
    }

    if (TagConversion::existsId3Tag("Track")) {
        std::stringstream   trackString;
        trackString << tag->track();
        value.reset(new const Glib::ustring(trackString.str()));
        setMetadata(value, TagConversion::id3ToDublinCore("Track"));
    }
    
    TagLib::MPEG::File      mpegFile(getUri()->c_str());
    TagLib::ID3v2::Tag*     id3v2Tag = mpegFile.ID3v2Tag();
    if (!id3v2Tag) {
        return;
    }
    
    TagLib::ID3v2::FrameListMap         frameListMap = id3v2Tag->frameListMap();
    TagLib::ID3v2::FrameListMap::ConstIterator it = frameListMap.begin();
    while (it != frameListMap.end()) {
        const char*     keyBuffer = it->first.data();
        std::string     keyString(keyBuffer, 4);
        if (TagConversion::existsId3Tag(keyString)) {
            TagLib::ID3v2::FrameList        frameList = it->second;
            if (!frameList.isEmpty()) {
                value.reset(new const Glib::ustring(frameList.front()
                                                    ->toString().to8Bit(true)));
                setMetadata(value, TagConversion::id3ToDublinCore(keyString));
            }
        }
        ++it;
    }
}


/*------------------------------------------------------------------------------
 *  Separate a key into the metadata name and its namespace
 *----------------------------------------------------------------------------*/
void
LiveSupport::Core :: separateNameAndNameSpace(const std::string & key,
                                                 std::string &       name,
                                                 std::string &       prefix)
                                                            throw ()
{
    unsigned int    colonPosition = key.find(':');

    if (colonPosition != std::string::npos) {               // there is a colon
        prefix   = key.substr(0, colonPosition);
        name     = key.substr(colonPosition+1);
    } else {                                                // no colon found
        prefix   = "";
        name     = key;
    }
}

