/**
 * Autogenerated by Thrift Compiler (0.7.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "spatial_ranker_types.h"



const char* SpatialRankerRequest::ascii_fingerprint = "AC78B4818B88D315545386C8B05969BB";
const uint8_t SpatialRankerRequest::binary_fingerprint[16] = {0xAC,0x78,0xB4,0x81,0x8B,0x88,0xD3,0x15,0x54,0x53,0x86,0xC8,0xB0,0x59,0x69,0xBB};

uint32_t SpatialRankerRequest::read(::apache::thrift::protocol::TProtocol* iprot) {

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
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->keypoints);
          this->__isset.keypoints = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->descriptors);
          this->__isset.descriptors = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            this->pics.clear();
            uint32_t _size0;
            ::apache::thrift::protocol::TType _etype3;
            iprot->readListBegin(_etype3, _size0);
            this->pics.resize(_size0);
            uint32_t _i4;
            for (_i4 = 0; _i4 < _size0; ++_i4)
            {
              xfer += iprot->readI64(this->pics[_i4]);
            }
            iprot->readListEnd();
          }
          this->__isset.pics = true;
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

uint32_t SpatialRankerRequest::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("SpatialRankerRequest");
  xfer += oprot->writeFieldBegin("keypoints", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->keypoints);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("descriptors", ::apache::thrift::protocol::T_STRING, 2);
  xfer += oprot->writeString(this->descriptors);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("pics", ::apache::thrift::protocol::T_LIST, 3);
  {
    xfer += oprot->writeListBegin(::apache::thrift::protocol::T_I64, static_cast<uint32_t>(this->pics.size()));
    std::vector<int64_t> ::const_iterator _iter5;
    for (_iter5 = this->pics.begin(); _iter5 != this->pics.end(); ++_iter5)
    {
      xfer += oprot->writeI64((*_iter5));
    }
    xfer += oprot->writeListEnd();
  }
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

const char* SpatialSimilar::ascii_fingerprint = "056BD45B5249CAA453D3C7B115F349DB";
const uint8_t SpatialSimilar::binary_fingerprint[16] = {0x05,0x6B,0xD4,0x5B,0x52,0x49,0xCA,0xA4,0x53,0xD3,0xC7,0xB1,0x15,0xF3,0x49,0xDB};

uint32_t SpatialSimilar::read(::apache::thrift::protocol::TProtocol* iprot) {

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
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->picid);
          this->__isset.picid = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_DOUBLE) {
          xfer += iprot->readDouble(this->sim);
          this->__isset.sim = true;
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

uint32_t SpatialSimilar::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("SpatialSimilar");
  xfer += oprot->writeFieldBegin("picid", ::apache::thrift::protocol::T_I64, 1);
  xfer += oprot->writeI64(this->picid);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldBegin("sim", ::apache::thrift::protocol::T_DOUBLE, 2);
  xfer += oprot->writeDouble(this->sim);
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

const char* SpatialRankerResponse::ascii_fingerprint = "716540467DF4D3A297965A5FF7A1AB34";
const uint8_t SpatialRankerResponse::binary_fingerprint[16] = {0x71,0x65,0x40,0x46,0x7D,0xF4,0xD3,0xA2,0x97,0x96,0x5A,0x5F,0xF7,0xA1,0xAB,0x34};

uint32_t SpatialRankerResponse::read(::apache::thrift::protocol::TProtocol* iprot) {

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
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            this->result.clear();
            uint32_t _size6;
            ::apache::thrift::protocol::TType _etype9;
            iprot->readListBegin(_etype9, _size6);
            this->result.resize(_size6);
            uint32_t _i10;
            for (_i10 = 0; _i10 < _size6; ++_i10)
            {
              xfer += this->result[_i10].read(iprot);
            }
            iprot->readListEnd();
          }
          this->__isset.result = true;
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

uint32_t SpatialRankerResponse::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("SpatialRankerResponse");
  xfer += oprot->writeFieldBegin("result", ::apache::thrift::protocol::T_LIST, 1);
  {
    xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRUCT, static_cast<uint32_t>(this->result.size()));
    std::vector<SpatialSimilar> ::const_iterator _iter11;
    for (_iter11 = this->result.begin(); _iter11 != this->result.end(); ++_iter11)
    {
      xfer += (*_iter11).write(oprot);
    }
    xfer += oprot->writeListEnd();
  }
  xfer += oprot->writeFieldEnd();
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


