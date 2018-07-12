/**
 * Autogenerated by Thrift Compiler (0.7.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "imgsearch_types.h"



const char* ImageSearchRequest::ascii_fingerprint = "3368C2F81F2FEF71F11EDACDB2A3ECEF";
const uint8_t ImageSearchRequest::binary_fingerprint[16] = {0x33,0x68,0xC2,0xF8,0x1F,0x2F,0xEF,0x71,0xF1,0x1E,0xDA,0xCD,0xB2,0xA3,0xEC,0xEF};

uint32_t ImageSearchRequest::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->type);
          this->__isset.type = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->key);
          this->__isset.key = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->workload);
          this->__isset.workload = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t ImageSearchRequest::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("ImageSearchRequest");
  xfer += oprot->writeFieldBegin("type", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32(this->type);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("key", ::apache::thrift::protocol::T_STRING, 2);
  xfer += oprot->writeString(this->key);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("workload", ::apache::thrift::protocol::T_STRING, 3);
  xfer += oprot->writeString(this->workload);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

const char* ImageSearchResponse::ascii_fingerprint = "52C6DAB6CF51AF617111F6D3964C6503";
const uint8_t ImageSearchResponse::binary_fingerprint[16] = {0x52,0xC6,0xDA,0xB6,0xCF,0x51,0xAF,0x61,0x71,0x11,0xF6,0xD3,0x96,0x4C,0x65,0x03};

uint32_t ImageSearchResponse::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->return_code);
          this->__isset.return_code = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readBinary(this->img_data);
          this->__isset.img_data = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->img_data_len);
          this->__isset.img_data_len = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t ImageSearchResponse::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("ImageSearchResponse");
  xfer += oprot->writeFieldBegin("return_code", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32(this->return_code);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("img_data", ::apache::thrift::protocol::T_STRING, 2);
  xfer += oprot->writeBinary(this->img_data);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("img_data_len", ::apache::thrift::protocol::T_I32, 3);
  xfer += oprot->writeI32(this->img_data_len);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


