/**
Copyright 2009-2018 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2018, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
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

#include <sstmac/hardware/pisces/pisces_param_expander.h>
#include <sstmac/hardware/logp/logp_param_expander.h>
#include <sstmac/common/timestamp.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/keyword_registration.h>

RegisterKeywords(
 { "buffer_size", "size of input/output buffers on network switches" },
);

namespace sstmac {
namespace hw {

void
PiscesParamExpander::expand(SST::Params& params)
{
  std::string amm_type = params->get_param("amm_model");
  if (amm_type == "amm4"){
    tiled_switch_ = true;
  } else {
    tiled_switch_ = false;
  } 

  SST::Params node_params = params->get_optional_namespace("node");
  SST::Params nic_params = node_params->get_optional_namespace("nic");
  SST::Params inj_params = nic_params->get_optional_namespace("injection");
  SST::Params mem_params = node_params->get_optional_namespace("memory");
  SST::Params switch_params = params->get_optional_namespace("switch");
  SST::Params top_params = params->get_optional_namespace("topology");
  SST::Params proc_params = node_params->get_optional_namespace("proc");


  nic_params->add_param_override("name", "pisces");
  switch_params->add_param_override("name", "pisces");
  if (!mem_params->has_scoped_param("name")){
    mem_params->add_param_override("name", "pisces");
  }

  buffer_depth_ = params->get_optional_int_param("network_buffer_depth", 8);
  //by default, quite coarse-grained

  std::string arb = params->get_optional_param("arbitrator", "cut_through");
  if (!switch_params->has_param("arbitrator")){
    switch_params->add_param("arbitrator", arb);
  }
  if (!nic_params->has_param("arbitrator")){
    nic_params->add_param("arbitrator",arb);
  }

  if (!mem_params->has_param("mtu")){
    int mem_packet_size = params->get_optional_int_param("memory_accuracy_parameter", 4096000);
    mem_params->add_param_override("mtu", mem_packet_size);
  }

  int packet_size = params->get_optional_int_param("accuracy_parameter", 4096);
  int net_packet_size = params->get_optional_int_param("network_accuracy_parameter", packet_size);
  if (!switch_params->has_param("mtu")){
    switch_params->add_param_override("mtu", net_packet_size);
  }
  if (!nic_params->has_param("mtu")){
    nic_params->add_param_override("mtu", net_packet_size);
  }

  if (amm_type == "amm1"){
    expandAmm1Memory(params, mem_params);
    expandAmm1Network(params, switch_params, true/*set xbar*/);
    expandAmm1Nic(params, nic_params);
  } else if (amm_type == "amm2"){
    expandAmm2Memory(params, mem_params);
    expandAmm1Network(params, switch_params, true/*set xbar*/);
    expandAmm1Nic(params, nic_params);
  } else if (amm_type == "amm3"){
    expandAmm2Memory(params, mem_params);
    expandAmm3Network(params, switch_params);
    expandAmm1Nic(params, nic_params);
  } else if (amm_type == "amm4"){
    expandAmm2Memory(params, mem_params);
    expandAmm4Network(params, top_params, switch_params);
    expandAmm4Nic(params, top_params, nic_params);
  } else {
    spkt_throw_printf(sprockit::input_error, "invalid hardware model %s given",
        amm_type.c_str());
  }
}

void
PiscesParamExpander::expandAmm1Memory(SST::Params& params,
                                      SST::Params& mem_params)
{
  if (mem_params->get_scoped_param("name") != "null"){
    mem_params->add_param_override("total_bandwidth", mem_params->get_param("bandwidth"));
  }
}

void
PiscesParamExpander::expandAmm1Network(SST::Params& params,
                                           SST::Params& switch_params,
                                           bool set_xbar)
{

  //JJW - no, don't do this
  //The link bandwidths will get multiplied during the connect
  //if redundant links, appropriately multiply the bandwidth
  //double bw_multiplier = network_bandwidth_multiplier(params);
  //double link_bw = switch_params->get_bandwidth_param("link_bandwidth");
  //if (bw_multiplier > 1.0001){
  //  link_bw *= bw_multiplier;
  //  switch_params->add_param_override("link_bandwidth", link_bw);
  //}

  SST::Params link_params = switch_params.get_namespace("link");
  SST::Params xbar_params = switch_params.get_namespace("xbar");
  SST::Params ej_params = switch_params->get_optional_namespace("ejection");
  SST::Params node_params = params.get_namespace("node");
  SST::Params nic_params = node_params.get_namespace("nic");
  SST::Params inj_params = nic_params.get_namespace("injection");

  std::string link_lat = link_params->get_either_or_param("sendLatency","latency");
  std::string xbar_lat = xbar_params->get_optional_param("latency", "0ns");

  if (!link_params->has_param("sendLatency")){
    if (link_lat.size() == 0){
      spkt_abort_printf("must specify latency or sendLatency for link");
    }
    link_params->add_param_override("sendLatency", link_lat);
  }
  if (!link_params->has_param("creditLatency")){
    link_params->add_param_override("creditLatency", link_params->get_param("sendLatency"));
  }

  if (!xbar_params->has_param("sendLatency")){
    xbar_params->add_param_override("sendLatency", xbar_lat);
  }
  if (!xbar_params->has_param("creditLatency")){
    xbar_params->add_param_override("creditLatency", link_lat);
  }

  //make the xbar much faster than links
  if (set_xbar){
    double link_bw = link_params->get_bandwidth_param("bandwidth");
    double xbar_bw = link_bw * buffer_depth_;
    xbar_params->add_param_override("bandwidth", xbar_bw);
    xbar_params->add_param_override("arbitrator", "null");
  }

  int buffer_size;
  if (switch_params->has_param("buffer_size")){
    buffer_size = switch_params->get_byte_length_param("buffer_size");
  } else {
    int size_multiplier = switchBufferMultiplier(params);
    int packet_size = params->get_optional_int_param("accuracy_parameter", 4096);
    buffer_size = buffer_depth_ * packet_size * size_multiplier;
    switch_params->add_param_override("buffer_size", buffer_size);
  }

  link_params->add_param_override("credits", buffer_size);

  if (!ej_params->has_param("sendLatency")){
    if (!ej_params->has_param("latency")){
      ej_params->add_param_override("sendLatency", inj_params->get_param("latency"));
    } else {
      ej_params->add_param_override("sendLatency",
                                    ej_params->get_param("latency"));
    }
  }
  if (!ej_params->has_param("bandwidth")){
    ej_params->add_param_override("bandwidth",
                 inj_params->get_param("bandwidth"));
  }

  ej_params["credits"].setByteLength(100, "GB");
  if (!ej_params->has_param("arbitrator")){
    ej_params->add_param("arbitrator", "cut_through");
  }
  if (!ej_params->has_param("creditLatency")){
    ej_params->add_param_override("creditLatency", "0ns");
  }
}

void
PiscesParamExpander::expandAmm1Nic(SST::Params& params,
                                   SST::Params& nic_params)
{
  SST::Params xbar_params = params.get_namespace("switch")->get_namespace("xbar");
  SST::Params inj_params = nic_params.get_namespace("injection");
  if (!inj_params->has_param("arbitrator")){
    inj_params->add_param("arbitrator", "cut_through");
  }

  int inj_red = inj_params->get_optional_int_param("redundant",1);
  int buf_size = xbar_params->get_byte_length_param("buffer_size");
  int inj_credits = buf_size * inj_red;
  inj_params["credits"].setByteLength(inj_credits, "B");
}

void
PiscesParamExpander::expandAmm2Memory(SST::Params& params,
                                      SST::Params& mem_params)
{
  expandAmm1Memory(params, mem_params);
  if (mem_params->get_scoped_param("name") != "null"){
    //mem_params->add_param_override("max_single_bandwidth",
    //                               params->get_param("max_memory_bandwidth"));
  }
}

void
PiscesParamExpander::expandAmm3Network(SST::Params& params,
                                       SST::Params& switch_params)
{
  expandAmm1Network(params, switch_params, false);
  SST::Params xbar_params = switch_params.get_namespace("xbar");
  double sw_bw = xbar_params->get_bandwidth_param("bandwidth");
  double bw_multiplier = switchBandwidthMultiplier(params);
  if (bw_multiplier > 1.0001){
    double xbar_bw = sw_bw * bw_multiplier;
    xbar_params->add_param_override("bandwidth", xbar_bw);
  }
}

void
PiscesParamExpander::expandAmm4Network(SST::Params& params,
  SST::Params& top_params,
  SST::Params& switch_params)
{
  tiled_switch_ = true;
  std::string top = top_params->get_param("name");
  std::string newtop = std::string("tiled_") + top;
  std::vector<int> switch_geom; switch_params->get_vector_param("geometry", switch_geom);
  if (switch_geom.size() != 2){
    sprockit::abort("AMM4: need switch geometry vector with 2 params:\n"
      "tiles-per-row, tiles-per-col");
  }
  int ncols = switch_geom[0];
  int nrows = switch_geom[1];
  top_params->add_param_override("tiles_per_row", ncols);
  top_params->add_param_override("tiles_per_col", nrows);
  top_params->add_param_override("name", newtop);

  switch_params->add_param_override("name", "pisces_tiled");

  SST::Params rtr_params = switch_params->get_optional_namespace("router");
  if (rtr_params->has_param("name")) {
    std::string router = rtr_params->get_param("name");
    std::string new_router = top + "_" + router + "_multipath";
    rtr_params->add_param_override("name", new_router);
  } else {
    spkt_throw_printf(sprockit::value_error,
                      "if using amm4, must specify switch.router.name = X\n"
                      "valid options are minimal, ugal, valiant, min_ad)");
  }

  switch_params->add_param_override("nrows", nrows);
  switch_params->add_param_override("ncols", ncols);

  int buffer_size;
  if (switch_params->has_param("buffer_size")){
    buffer_size = switch_params->get_byte_length_param("buffer_size");
  } else {
    int size_multiplier = switchBufferMultiplier(params);
    int packet_size = params->get_optional_int_param("accuracy_parameter", 4096);
    buffer_size = buffer_depth_ * packet_size * size_multiplier;
    switch_params->add_param_override("buffer_size", buffer_size);
  }

  // expand amm1 network params
  SST::Params link_params = switch_params.get_namespace("link");
  SST::Params xbar_params = switch_params.get_namespace("xbar");
  SST::Params demux_params = switch_params->get_optional_namespace("input");
  SST::Params ej_params = switch_params->get_optional_namespace("ejection");
  SST::Params node_params = params.get_namespace("node");
  SST::Params nic_params = node_params.get_namespace("nic");
  SST::Params inj_params = nic_params.get_namespace("injection");

  std::string link_lat = link_params->get_optional_param("latency","");

  // [demuxer -> xbar -> muxer] -> [demuxer...]

  if (!link_params->has_param("sendLatency")){
    if (link_lat.size() == 0) {
      spkt_abort_printf("must specify latency or sendLatency for link");
    }
    link_params->add_param_override("sendLatency", link_lat);
  }
  if (!link_params->has_param("creditLatency")){
    link_params->add_param_override("creditLatency", link_params->get_param("sendLatency"));
  }

  if (!xbar_params->has_param("sendLatency"))
    xbar_params->add_param_override("sendLatency", "0ns");
  if (!xbar_params->has_param("creditLatency"))
    xbar_params->add_param_override("creditLatency", "0ns");

  if (!demux_params->has_param("sendLatency"))
    demux_params->add_param_override("sendLatency", "0ns");
  if (!demux_params->has_param("creditLatency"))
    demux_params->add_param_override("creditLatency", link_params->get_param("sendLatency"));

  if (!demux_params->has_param("bandwidth"))
    demux_params->add_param_override("bandwidth", xbar_params->get_param("bandwidth"));

  // link params get passed to output muxer by interconnect setup routines
  // but xbar and input demuxer need to be configured directly here
  buffer_size = xbar_params->get_byte_length_param("buffer_size");
  std::string arb = params->get_optional_param("arbitrator", "cut_through");
  link_params->add_param_override("credits", buffer_size);
  xbar_params->add_param_override("credits", buffer_size);
  xbar_params->add_param("arbitrator",arb);
  demux_params->add_param_override("credits", buffer_size);
  demux_params->add_param("arbitrator",arb);

  if (!ej_params->has_param("sendLatency")){
    if (!ej_params->has_param("latency")){
      ej_params->add_param_override("sendLatency",
                                    link_params->get_param("sendLatency"));
    } else {
      ej_params->add_param_override("sendLatency",
                                    ej_params->get_param("latency"));
    }
  }
  if (!ej_params->has_param("bandwidth")){
    ej_params->add_param_override("bandwidth",
                                  link_params->get_bandwidth_param("bandwidth"));
  }

  ej_params["credits"].setByteLength(100, "GB");
  if (!ej_params->has_param("arbitrator")){
    ej_params->add_param("arbitrator", "cut_through");
  }
  ej_params->add_param_override("creditLatency", "0ns");
}

void
PiscesParamExpander::expandAmm4Nic(SST::Params& params,
                                   SST::Params& top_params,
                                   SST::Params& nic_params)
{
  // set arb and number of credits on nic
  expandAmm1Nic(params, nic_params);
}

}
}
