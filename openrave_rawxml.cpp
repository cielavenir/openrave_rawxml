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

#define BOOST_SYSTEM_NO_DEPRECATED
#include <openrave/openrave.h>
#include <openrave/xmlreaders.h>
#include <openrave/openravejson.h>
#include <string>
#include <sstream>
#include <vector>

#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<OpenRAVE::Readable> ReadablePtr;

std::vector<OpenRAVE::UserDataPtr> vRegisteredReaders;

namespace OpenRAVE {
	/// XML ///
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
					for(auto &it: _atts) {
						stream << " " << it.first << "=\"" << it.second << "\"";
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
				for(auto &it: _listchildren) {
					it->Serialize(stream);
				}
				if( _xmltag.size() > 0 ) {
					stream << "</" << _xmltag << ">";// << std::endl;
				}
			}
		};
		typedef boost::shared_ptr<OpenRAVE::xmlreaders::StreamXMLWriterLessSerialize> StreamXMLWriterLessSerializePtr;

		class OPENRAVE_API XMLTransferReader : public BaseXMLReader
		{
			protected:
			std::vector<BaseXMLWriterPtr> stwriter;
			std::vector<std::string> sttag;
			public:
			XMLTransferReader(BaseXMLWriterPtr writer){stwriter.push_back(writer);}
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
		typedef boost::shared_ptr<OpenRAVE::xmlreaders::XMLTransferReader> XMLTransferReaderPtr;

		class OPENRAVE_API RawXMLReadable : public Readable
		{
			std::string _data;
			std::string _profile;
			public:
			RawXMLReadable(const std::string& xmlid, const std::string& data, const std::string& profile="OpenRAVE") : Readable(xmlid), _data(data), _profile(profile)
			{
			}
			virtual bool SerializeXML(BaseXMLWriterPtr writer, int options) const
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
				XMLTransferReader trans(writer);
				std::string data = std::string("<XMLTRANSFER_ARRAY_TOP>")+_data+"</XMLTRANSFER_ARRAY_TOP>";
				LocalXML::ParseXMLData(trans,data.c_str(),data.size());
				return true;
			}
			virtual bool SerializeJSON(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator, dReal fUnitScale, int options) const
			{
				return false;
			}
			virtual bool DeserializeJSON(const rapidjson::Value& value, dReal fUnitScale)
			{
				return false;
			}
			virtual bool operator==(const Readable& other) const
			{
				// pointer value
				return this == &other;
			}
                        virtual bool operator==(const Readable& other)
                        {
                                return *this == other;
                        }
			virtual ReadablePtr CloneSelf() const
			{
				boost::shared_ptr<RawXMLReadable> pNew(new RawXMLReadable(GetXMLId(), _data, _profile));
				*pNew = *this;
				return pNew;
			}
		};
		typedef boost::shared_ptr<OpenRAVE::xmlreaders::RawXMLReadable> RawXMLReadablePtr;

		// not OPENRAVE_API
		class XMLTransferStreamSerialize : public XMLTransferReader
		{
			StreamXMLWriterLessSerializePtr streamwriter;
			const std::string xmlid;
			std::string profile;
			int techniquecnt;
			public:
			XMLTransferStreamSerialize(StreamXMLWriterLessSerializePtr writer, const std::string& _xmlid):XMLTransferReader(writer), streamwriter(writer), profile("OpenRAVE"), xmlid(_xmlid), techniquecnt(0) {}

			virtual ProcessElement startElement(const std::string& xmlname_orig, const AttributesList& atts)
			{
				std::string xmlname;
				xmlname.resize(xmlname_orig.size());
				std::transform(xmlname_orig.begin(), xmlname_orig.end(), xmlname.begin(), toupper);

				if(xmlname=="TECHNIQUE"){
					if(techniquecnt++==0){
						for(auto &itatt: atts){
							if( itatt.first == "profile" ){
								profile = itatt.second;
							}
						}
						return PE_Support;
					}
				}
				return XMLTransferReader::startElement(xmlname_orig,atts);
			}

			virtual bool endElement(const std::string& xmlname_orig)
			{
				std::string xmlname;
				xmlname.resize(xmlname_orig.size());
				std::transform(xmlname_orig.begin(), xmlname_orig.end(), xmlname.begin(), toupper);

				if(xmlname=="TECHNIQUE"){
					if(--techniquecnt==0)return true;
				}
				return XMLTransferReader::endElement(xmlname_orig);
			}

			virtual ReadablePtr GetReadable() {
				std::ostringstream ss;
				streamwriter->Serialize(ss);
				return RawXMLReadablePtr(new RawXMLReadable(xmlid,ss.str(),profile));
			}
		};
		typedef boost::shared_ptr<OpenRAVE::xmlreaders::XMLTransferStreamSerialize> XMLTransferStreamSerializePtr;

		// not OPENRAVE_API
		class XMLExtraFieldAcceptorFactory
		{
			const std::string xmlid;
			public:
			XMLExtraFieldAcceptorFactory(const std::string& _xmlid) : xmlid(_xmlid) {}
			BaseXMLReaderPtr operator() (InterfaceBasePtr ptr, const AttributesList& atts) {
				StreamXMLWriterLessSerializePtr writer(new StreamXMLWriterLessSerialize("",atts)); // not xmlid
				return XMLTransferStreamSerializePtr(new XMLTransferStreamSerialize(writer,xmlid));
			}
		};
	}

	/// JSON ///
	class OPENRAVE_API RawJSONReadable : public Readable
	{
		std::string _data;
		mutable std::vector<rapidjson::Document> _docs;
		public:
		RawJSONReadable(const std::string& xmlid, const std::string& data) : Readable(xmlid), _data(data)
		{
		}
		RawJSONReadable(const std::string& xmlid) : Readable(xmlid), _data("")
		{
		}
		virtual ~RawJSONReadable()
		{
			_docs.clear();
		}
		virtual bool SerializeXML(BaseXMLWriterPtr writer, int options) const
		{
			return false;
		}
		virtual bool SerializeJSON(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator, dReal fUnitScale, int options) const
		{
			{
				// todo properly parse using "allocator" argument
				_docs.push_back(rapidjson::Document());
				rapidjson::Document &doc = _docs.back();
				orjson::ParseJson(doc,_data);
				value.Swap(doc);
				// puts(orjson::DumpJson(value).c_str());
				// _docs.clear(); // todo this line causes memory corruption //
			}
			// puts(orjson::DumpJson(value).c_str());
			return true;
		}
		virtual bool DeserializeJSON(const rapidjson::Value& value, dReal fUnitScale)
		{
			_data = orjson::DumpJson(value);
			return true;
		}
		virtual bool operator==(const Readable& other) const
		{
			// pointer value
			return this == &other;
		}
		virtual bool operator==(const Readable& other)
		{
			return *this == other;
		}
		virtual ReadablePtr CloneSelf() const
		{
			/// please do not use this ///
			boost::shared_ptr<RawJSONReadable> pNew(new RawJSONReadable(GetXMLId(), _data));
			//*pNew = *this;
			return pNew;
		}
	};
	typedef boost::shared_ptr<OpenRAVE::RawJSONReadable> RawJSONReadablePtr;

	// not OPENRAVE_API
	class JSONTransferReader : public BaseJSONReader
	{
		RawJSONReadablePtr readable;
		public:
		JSONTransferReader(const std::string &xmlid) : readable(new RawJSONReadable(xmlid))
		{
		}
		virtual ReadablePtr GetReadable() {
			return readable;
		}
	};
	typedef boost::shared_ptr<OpenRAVE::JSONTransferReader> JSONTransferReaderPtr;

	// not OPENRAVE_API
	class JSONExtraFieldAcceptorFactory
	{
		const std::string xmlid;
		public:
		JSONExtraFieldAcceptorFactory(const std::string& _xmlid) : xmlid(_xmlid) {}
		BaseJSONReaderPtr operator() (ReadablePtr ptr, const AttributesList& atts) {
			return JSONTransferReaderPtr(new JSONTransferReader(xmlid));
		}
	};

	void OPENRAVE_API AcceptExtraField(InterfaceType type, const std::string& xmlid){
		::vRegisteredReaders.push_back(RaveRegisterXMLReader(type,xmlid,xmlreaders::XMLExtraFieldAcceptorFactory(xmlid)));
		::vRegisteredReaders.push_back(RaveRegisterJSONReader(type,xmlid,JSONExtraFieldAcceptorFactory(xmlid)));
	}
}

#include <openravepy/bindings.h>
#ifdef USE_PYBIND11_PYTHON_BINDINGS
#define PYDEF(fndef) m.fndef
#else
#define PYDEF(fndef) fndef
#endif
//namespace py = openravepy::py;

namespace openravepy {
	py::object toPyReadable(ReadablePtr p);
	py::object pyCreateRawXMLReadable(const std::string& xmlid, const std::string& data, const std::string& profile="OpenRAVE")
	{
		return toPyReadable(ReadablePtr(new OpenRAVE::xmlreaders::RawXMLReadable(xmlid, data, profile)));
	}
	py::object pyCreateRawJSONReadable(const std::string& xmlid, const std::string& data)
	{
		return toPyReadable(ReadablePtr(new OpenRAVE::RawJSONReadable(xmlid, data)));
	}

	//BOOST_PYTHON_MODULE(openrave_rawxml)
	OPENRAVE_PYTHON_MODULE(openrave_rawxml)
	{
		PYDEF(def("CreateRawXMLReadable",pyCreateRawXMLReadable, PY_ARGS("xmlid", "data", "profile") ""));
		PYDEF(def("CreateRawJSONReadable",pyCreateRawJSONReadable, PY_ARGS("xmlid", "data") ""));
		PYDEF(def("AcceptExtraField",OpenRAVE::AcceptExtraField, PY_ARGS("type", "xmlid") ""));
	}
}
