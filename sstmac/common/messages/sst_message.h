/**
Copyright 2009-2017 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2017, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Sandia Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#ifndef SSTMAC_COMMON_MESSAGES_SST_MESSAGE_H_INCLUDED
#define SSTMAC_COMMON_MESSAGES_SST_MESSAGE_H_INCLUDED


#include <sstmac/common/serializable.h>
#include <sstmac/common/node_address.h>
#include <sprockit/metadata_bits.h>
#include <sstmac/common/sst_event.h>


namespace sstmac {

class flow :
  public event,
  public sprockit::printable
{
 public:
  /**
   * Virtual function to return size. Child classes should impement this
   * if they want any size tracked / modeled.
   * @return Zero size, meant to be implemented by children.
   */
  virtual long byte_length() const = 0;

  uint64_t flow_id() const {
    return flow_id_;
  }

  void set_flow_id(uint64_t id) {
    flow_id_ = id;
  }

  void serialize_order(serializer& ser){
    ser & flow_id_;
    event::serialize_order(ser);
  }

  void clone_into(flow* flw) const {
    flw->flow_id_ = flow_id_;
  }

 protected:
  flow(){} //for serialization

 private:
  uint64_t flow_id_;
};

/**
 * A class describing an event.
 */
class message :
  public flow
{
 public:
  virtual ~message() {}

  node_id toaddr() const {
    return toaddr_;
  }

  node_id fromaddr() const {
    return fromaddr_;
  }

  void set_toaddr(node_id addr) {
    toaddr_ = addr;
  }

  void set_fromaddr(node_id addr) {
    fromaddr_ = addr;
  }


  virtual bool needs_ack() const {
    return false;
  }

  void serialize_order(serializer& ser){
    flow::serialize_order(ser);
    ser & toaddr_;
    ser & fromaddr_;
  }

  void clone_into(message* msg) const {
    flow::clone_into(msg);
    msg->toaddr_ = toaddr_;
    msg->fromaddr_ = fromaddr_;
  }

  virtual bool is_bcast() const {
    return false;
  }

  virtual message* clone_ack() const {
    return nullptr;
  }

 protected:
  message(){} //for serialization

  message(node_id from, node_id to) :
    toaddr_(to), fromaddr_(from)
  {
  }

 private:
  node_id toaddr_;
  node_id fromaddr_;

};





} // end of namespace sstmac
#endif
