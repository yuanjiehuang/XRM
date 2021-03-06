/*
 * Copyright (C) 2019-2020, Xilinx Inc - All rights reserved
 * Xilinx Resouce Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include "xrm_tcp_session.hpp"

void xrm::session::doRead() {
    auto self(shared_from_this());
    m_socket.async_read_some(boost::asio::buffer(m_indata, max_length),
                             [this, self](boost::system::error_code const& ec, std::size_t length) {
                                 if (ec) {
                                     m_system->logMsg(XRM_LOG_DEBUG, "doRead(): ec %s = %d", ec.category().name(),
                                                      ec.value());
                                 }
                                 if ((boost::asio::error::eof == ec) || (boost::asio::error::connection_reset == ec)) {
                                     uint64_t clientId = this->getClientId();
                                     if (clientId) m_system->recycleResource(clientId);
                                 }
                                 if (length == 0) {
                                     m_system->logMsg(XRM_LOG_DEBUG, "doRead(): receive 0 length on read, ignored");
                                 } else {
                                     handleCmd(length);
                                 }
                             });
}

void xrm::session::doWrite(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(m_socket, boost::asio::buffer(m_outdata, length),
                             [this, self](boost::system::error_code const& ec, std::size_t /*length*/) {
                                 if (ec) {
                                     m_system->logMsg(XRM_LOG_DEBUG, "doWrite(): ec %s = %d", ec.category().name(),
                                                      ec.value());
                                     uint64_t clientId = this->getClientId();
                                     if (clientId) m_system->recycleResource(clientId);
                                 } else {
                                     doRead();
                                 }
                             });
}

void xrm::session::handleCmd(std::size_t length) {
    auto self(shared_from_this());

    std::stringstream instr;
    std::stringstream outstr;
    std::string name, strRequestId, echoClientId;

    m_indata[length] = 0;
    instr << m_indata;
    boost::property_tree::ptree outrsp;
    try {
        boost::property_tree::read_json(instr, m_cmdtree);
    } catch (const boost::property_tree::json_parser_error& e) {
        outrsp.put("response.status", "failed");
        outrsp.put("response.data.failed", "Input Json file format error: " + e.message());
        goto end_of_cmd;
    }

    name = m_cmdtree.get<std::string>("request.name", "");
    if (name.c_str()[0] == '\0') {
        outrsp.put("response.status", "failed");
        outrsp.put("response.data.failed", "request name is not provided");
        goto end_of_cmd;
    }

    strRequestId = m_cmdtree.get<std::string>("request.requestId", "");
    if (strRequestId.c_str()[0] == '\0') {
        outrsp.put("response.status", "failed");
        outrsp.put("response.data.failed", "request requestId is not provided");
        goto end_of_cmd;
    }

    /* The cmd received from xrm library will contain the clientId, so it can be used to trace
     * whether the session broken or not.
     * The cmd from xrmadm will NOT contain cliendId, and there is no need to trace the session.
     */
    echoClientId = m_cmdtree.get<std::string>("request.parameters.echoClientId", "");
    if (echoClientId.c_str()[0] != '\0')
        m_clientId = m_cmdtree.get<uint64_t>("request.parameters.clientId");
    else
        m_clientId = 0;
    m_registry->dispatch(name, m_cmdtree, outrsp);

end_of_cmd:
    boost::property_tree::write_json(outstr, outrsp);
    std::strcpy(m_outdata, outstr.str().c_str());
    doWrite(outstr.str().length());
}
