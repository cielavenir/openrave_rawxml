// This file is part of OpenRAVE.
// OpenRAVE is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <openrave/openrave.h>
#include <openrave/xmlreaders.h>
#include <boost/python.hpp>
#include <string>
#include <sstream>
#include <vector>
#define FOREACH(it, v) for(typeof((v).begin())it = (v).begin(); it != (v).end(); (it)++)

std::vector<OpenRAVE::UserDataPtr> vRegisteredReaders;

namespace OpenRAVE {
	namespace LocalXML {
		bool ParseXMLData(OpenRAVE::BaseXMLReader& reader, const char* buffer, int size);
	}
	namespace xmlreaders {
		// not OPENRAVE_API
		class StreamXMLWriterLessSerialize : public StreamXMLWriter
		{
			public:
			StreamXMLWriterLessSerialize(const std::string& xmltag, const AttributesList& atts=AttributesList()) : StreamXMLWriter(xmltag, atts) {}
			virtual BaseXMLWriterPtr AddChild(const std::string& xmltag, const AttributesList& atts)
			{
				boost::shared_ptr<StreamXMLWriterLessSerialize> child(new StreamXMLWriterLessSerialize(xmltag,atts));
				_listchildren.push_back(child);
				return child;
			}
			virtual void Serialize(std::ostream& stream)
			{
				if( _xmltag.size() > 0 ) {
					stream << "<" << _xmltag;
					FOREACH(it, _atts) {
						stream << " " << it->first << "=\"" << it->second << "\"";
					}
					// don't skip any lines since could affect reading back _data
					stream << ">";
				}
				if( _data.size() > 0 ) {
					if( _xmltag.size() > 0 ) {
						//stream << "<![CDATA[" << _data << "]]>";
						stream << _data;
					}
					else {
						// there's no tag, so can render plaintext
						stream << _data;
					}
				}
				FOREACH(it, _listchildren) {
					(*it)->Serialize(stream);
				}
				if( _xmltag.size() > 0 ) {
					stream << "</" << _xmltag << ">";// << std::endl;
				}
			}
		};
		typedef boost::shared_ptr<OpenRAVE::xmlreaders::StreamXMLWriterLessSerialize> StreamXMLWriterLessSerializePtr;

		class OPENRAVE_API XMLTransfer : public BaseXMLReader
		{
			protected:
			std::vector<BaseXMLWriterPtr> stwriter;
			std::vector<std::string> sttag;
			public:
			XMLTransfer(BaseXMLWriterPtr writer){stwriter.push_back(writer);}
			virtual ProcessElement startElement(const std::string& xmlname_orig, const AttributesList& atts)
			{
				std::string xmlname;
				xmlname.resize(xmlname_orig.size());
				std::transform(xmlname_orig.begin(), xmlname_orig.end(), xmlname.begin(), toupper);

				if(xmlname!="XMLTRANSFER_ARRAY_TOP"){
					stwriter.push_back(stwriter.back()->AddChild(xmlname_orig,atts));
					sttag.push_back(xmlname_orig);
				}
				return PE_Support;
			}

			virtual bool endElement(const std::string& xmlname_orig)
			{
				std::string xmlname;
				xmlname.resize(xmlname_orig.size());
				std::transform(xmlname_orig.begin(), xmlname_orig.end(), xmlname.begin(), toupper);

				if(xmlname!="XMLTRANSFER_ARRAY_TOP"){
					if(stwriter.empty()||sttag.empty()||sttag.back()!=xmlname){
						// todo error
					}
					sttag.pop_back();
					stwriter.pop_back();
				}
				return false;
			}

			virtual void characters(const std::string& ch)
			{
				int i=0;
				for(;i<ch.size();i++){
					if(ch[i]!=' '&&ch[i]!='\n'&&ch[i]!='\r'&&ch[i]!='\t')break;
				}
				if(i<ch.size())stwriter.back()->SetCharData(ch);
			}
		};
		typedef boost::shared_ptr<OpenRAVE::xmlreaders::XMLTransfer> XMLTransferPtr;

		class OPENRAVE_API RawXMLReadable : public XMLReadable
		{
			std::string _data;
			std::string _profile;
			public:
			RawXMLReadable(const std::string& xmlid, const std::string& data, const std::string& profile="OpenRAVE") : XMLReadable(xmlid), _data(data), _profile(profile)
			{
			}
			void Serialize(BaseXMLWriterPtr writer, int options) const
			{
				if( writer->GetFormat() == "collada" ) {
					AttributesList atts;
					//atts.push_back(make_pair(std::string("type"),"xmlreadable"));
					atts.push_back(make_pair(std::string("type"), GetXMLId()));
					BaseXMLWriterPtr child = writer->AddChild("extra",atts);
					atts.clear();
					atts.push_back(make_pair(std::string("profile"), _profile));
					writer = child->AddChild("technique",atts);
				}
				XMLTransfer trans(writer);
				std::string data = std::string("<XMLTRANSFER_ARRAY_TOP>")+_data+"</XMLTRANSFER_ARRAY_TOP>";
				LocalXML::ParseXMLData(trans,data.c_str(),data.size());
			}
		};
		typedef boost::shared_ptr<OpenRAVE::xmlreaders::RawXMLReadable> RawXMLReadablePtr;

		// not OPENRAVE_API
		class XMLTransferStreamSerialize : public XMLTransfer
		{
			StreamXMLWriterLessSerializePtr streamwriter;
			const std::string xmlid;
			std::string profile;
			int techniquecnt;
			public:
			XMLTransferStreamSerialize(StreamXMLWriterLessSerializePtr writer, const std::string& _xmlid):XMLTransfer(writer), streamwriter(writer), profile("OpenAVE"), xmlid(_xmlid), techniquecnt(0) {}

			virtual ProcessElement startElement(const std::string& xmlname_orig, const AttributesList& atts)
			{
				std::string xmlname;
				xmlname.resize(xmlname_orig.size());
				std::transform(xmlname_orig.begin(), xmlname_orig.end(), xmlname.begin(), toupper);

				if(xmlname=="TECHNIQUE"){
					if(techniquecnt++==0){
						FOREACH(itatt,atts){
							if( itatt->first == "profile" ){
								profile = itatt->second;
							}
						}
						return PE_Support;
					}
				}
				return XMLTransfer::startElement(xmlname_orig,atts);
			}

			virtual bool endElement(const std::string& xmlname_orig)
			{
				std::string xmlname;
				xmlname.resize(xmlname_orig.size());
				std::transform(xmlname_orig.begin(), xmlname_orig.end(), xmlname.begin(), toupper);

				if(xmlname=="TECHNIQUE"){
					if(--techniquecnt==0)return true;
				}
				return XMLTransfer::endElement(xmlname_orig);
			}

			virtual XMLReadablePtr GetReadable() {
				std::ostringstream ss;
				streamwriter->Serialize(ss);
				return RawXMLReadablePtr(new RawXMLReadable(xmlid,ss.str(),profile));
			}
		};
		typedef boost::shared_ptr<OpenRAVE::xmlreaders::XMLTransferStreamSerialize> XMLTransferStreamSerializePtr;

		// not OPENRAVE_API
		class ExtraFieldAcceptorFactory
		{
			const std::string xmlid;
			public:
			ExtraFieldAcceptorFactory(const std::string& _xmlid) : xmlid(_xmlid) {}
			BaseXMLReaderPtr operator() (InterfaceBasePtr ptr, const AttributesList& atts) {
				StreamXMLWriterLessSerializePtr writer(new StreamXMLWriterLessSerialize("",atts)); // not xmlid
				return XMLTransferStreamSerializePtr(new XMLTransferStreamSerialize(writer,xmlid));
			}
		};

		void OPENRAVE_API AcceptExtraField(InterfaceType type, const std::string& xmlid){
			::vRegisteredReaders.push_back(RaveRegisterXMLReader(type,xmlid,ExtraFieldAcceptorFactory(xmlid)));
		}
	}
}

using namespace boost::python;
typedef boost::shared_ptr<OpenRAVE::XMLReadable> XMLReadablePtr;
namespace openravepy {
	object toPyXMLReadable(XMLReadablePtr p);
	object pyCreateRawXMLReadable(const std::string& xmlid, const std::string& data, const std::string& profile="OpenRAVE")
	{
		return toPyXMLReadable(XMLReadablePtr(new OpenRAVE::xmlreaders::RawXMLReadable(xmlid, data, profile)));
	}

	BOOST_PYTHON_MODULE(openrave_rawxml)
	{
		def("CreateRawXMLReadable",pyCreateRawXMLReadable, args("xmlid", "data", "profile"));
		def("AcceptExtraField",OpenRAVE::xmlreaders::AcceptExtraField, args("type", "xmlid"));
	}
}
