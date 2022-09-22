// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <AuthHandler.hpp>
#include <everest/logging.hpp>

namespace module {

AuthHandler::AuthHandler(const SelectionAlgorithm& selection_algorithm, const int connection_timeout,
                         bool prioritize_authorization_over_stopping_transaction) :
    selection_algorithm(selection_algorithm),
    connection_timeout(connection_timeout),
    prioritize_authorization_over_stopping_transaction(prioritize_authorization_over_stopping_transaction){};

AuthHandler::~AuthHandler() {

    for (const auto& connector_entry : this->connectors) {
        connector_entry.second->connector.state_machine.controller->stop();
    }
}

void AuthHandler::init_connector(const int connector_id, const int evse_index) {
    std::unique_ptr<ConnectorContext> ctx = std::make_unique<ConnectorContext>(connector_id, evse_index);
    this->connectors.emplace(connector_id, std::move(ctx));
    this->reservation_handler.init_connector(connector_id);
}

void AuthHandler::on_token(const ProvidedIdToken& provided_token) {
    // check if token is already currently processed
    EVLOG_info << "Received new token: " << provided_token;
    this->token_in_process_mutex.lock();
    if (!this->is_token_already_in_process(provided_token.id_token)) {
        // process token if not already in process
        const auto referenced_connectors = this->get_referenced_connectors(provided_token);
        this->tokens_in_process.insert(provided_token.id_token);
        this->token_in_process_mutex.unlock();
        this->handle_token(provided_token);
        this->unlock_referenced_connectors(referenced_connectors);
    } else {
        // do nothing if token is currently processed
        EVLOG_info << "Received token " << provided_token.id_token << " repeatedly while still processing it";
        this->token_in_process_mutex.unlock();
        return;
    }
    this->token_in_process_mutex.lock();
    this->tokens_in_process.erase(provided_token.id_token);
    this->token_in_process_mutex.unlock();
}

void AuthHandler::handle_token(const ProvidedIdToken& provided_token) {
    std::vector<int> referenced_connectors = this->get_referenced_connectors(provided_token);

    // check if id_token is used for an active transaction
    const auto connector_used_for_transaction =
        this->used_for_transaction(referenced_connectors, provided_token.id_token);
    if (connector_used_for_transaction != -1) {
        this->stop_transaction_callback(this->connectors.at(connector_used_for_transaction)->evse_index,
                                        StopTransactionReason::Local);
        EVLOG_info << "Transaction was stopped because id_token was used for transaction";
        return;
    }

    // validate
    std::vector<ValidationResult> validation_results;
    // only validate if token is not prevalidated
    if (provided_token.prevalidated && provided_token.prevalidated.value()) {
        ValidationResult validation_result;
        validation_result.authorization_status = AuthorizationStatus::Accepted;
        validation_results.push_back(validation_result);
    } else {
        validation_results = this->validate_token_callback(provided_token.id_token);
    }

    bool attempt_stop_with_parent_id_token = false;
    if (this->prioritize_authorization_over_stopping_transaction) {
        // check if any connector is available
        if (!this->any_connector_available(referenced_connectors)) {
            // check if parent_id_token can be used to finish transaction
            attempt_stop_with_parent_id_token = true;
        }
    } else {
        attempt_stop_with_parent_id_token = true;
    }

    if (attempt_stop_with_parent_id_token) {
        for (const auto& validation_result : validation_results) {
            if (validation_result.parent_id_token.has_value()) {
                const auto connector_used_for_transaction =
                    this->used_for_transaction(referenced_connectors, validation_result.parent_id_token.value());
                if (connector_used_for_transaction != -1) {
                    this->stop_transaction_callback(this->connectors.at(connector_used_for_transaction)->evse_index,
                                                    StopTransactionReason::Local);
                    EVLOG_info << "Transaction was stopped because parent_id_token was used for transaction";
                    return;
                }
            }
        }
    }

    // check if any connector is available
    if (!this->any_connector_available(referenced_connectors)) {
        return;
    }

    if (!validation_results.empty()) {
        bool authorized = false;
        int i = 0;
        // iterate over validation results
        while (i < validation_results.size() && !authorized) {
            auto validation_result = validation_results.at(i);
            if (validation_result.authorization_status == AuthorizationStatus::Accepted) {
                int connector_id = this->select_connector(referenced_connectors); // might block
                EVLOG_debug << "Selected connector#" << connector_id << " for token: " << provided_token.id_token;
                if (connector_id != -1) { // indicates timeout
                    const auto identifier = this->get_identifier(validation_result, provided_token.id_token);
                    if (!this->connectors.at(connector_id)->connector.reserved) {
                        EVLOG_info << "Providing authorization to connector#" << connector_id;
                        this->authorize_evse(connector_id, identifier);
                        authorized = true;
                    } else {
                        EVLOG_debug << "Connector is reserved. Checking if token matches...";
                        if (this->reservation_handler.matches_reserved_identifier(connector_id, provided_token.id_token,
                                                                                  validation_result.parent_id_token)) {
                            EVLOG_info << "Connector is reserved and token is valid for this reservation";
                            this->reservation_handler.on_reservation_used(connector_id);
                            this->authorize_evse(connector_id, identifier);
                            authorized = true;
                        } else {
                            EVLOG_info << "Connector is reserved but token is not valid for this reservation";
                        }
                    }
                } else {
                    EVLOG_info << "Timeout while selecting connector for provided token: " << provided_token;
                    return;
                }
            }
            i++;
        }
        if (!authorized) {
            EVLOG_debug << "id_token could not be validated by any validator";
        }
    } else {
        EVLOG_warning << "No validation result was received by any validator.";
    }
}

std::vector<int> AuthHandler::get_referenced_connectors(const ProvidedIdToken& provided_token) {
    std::vector<int> connectors;

    // either insert the given connector references of the provided token
    if (provided_token.connectors) {
        std::copy_if(provided_token.connectors.value().begin(), provided_token.connectors.value().end(),
                     std::back_inserter(connectors), [this](int connector_id) {
                         if (this->connectors.find(connector_id) != this->connectors.end()) {
                             return !this->connectors.at(connector_id)->connector.is_unavailable();
                         } else {
                             EVLOG_warning << "Provided token included references to connector_id that does not exist";
                             return false;
                         }
                     });
    }
    // or if there is no reference to connectors take all connectors
    else {
        for (const auto& entry : this->connectors) {
            if (!entry.second->connector.is_unavailable()) {
                connectors.push_back(entry.first);
            }
        }
    }
    return connectors;
}

int AuthHandler::used_for_transaction(const std::vector<int>& connector_ids, const std::string& token) {
    for (const auto connector_id : connector_ids) {
        if (this->connectors.at(connector_id)->connector.identifier.has_value()) {
            const auto& identifier = this->connectors.at(connector_id)->connector.identifier.value();
            // check against id_token
            if (identifier.id_token == token) {
                return connector_id;
            }
            // check against parent_id_token
            else if (identifier.parent_id_token.has_value() && identifier.parent_id_token.get() == token) {
                return connector_id;
            }
        }
    }
    return -1;
}

bool AuthHandler::is_token_already_in_process(const std::string& id_token) {
    // checks if the token is currently already processed by the module (because already swiped)
    return this->tokens_in_process.find(id_token) != this->tokens_in_process.end();
}

bool AuthHandler::any_connector_available(const std::vector<int>& connector_ids) {
    EVLOG_debug << "Checking availability of connectors...";
    for (const auto connector_id : connector_ids) {
        const auto state = this->connectors.at(connector_id)->connector.get_state();
        if (state != ConnectorState::UNAVAILABLE && state != ConnectorState::OCCUPIED &&
            state != ConnectorState::FAULTED) {
            EVLOG_debug << "There is at least one connector available";
            return true;
        }
    }
    EVLOG_debug << "No connector is available for this id_token";
    return false;
}

int AuthHandler::get_latest_plugin(const std::vector<int>& connectors) {
    for (const auto connector : this->plug_in_queue) {
        if (std::find(connectors.begin(), connectors.end(), connector) != connectors.end()) {
            return connector;
        }
    }
    return -1;
}

void AuthHandler::lock_referenced_connectors(const std::vector<int>& connectors) {
    for (const auto connector_id : connectors) {
        this->connectors.at(connector_id)->mutex.lock();
    }
}

void AuthHandler::unlock_referenced_connectors(const std::vector<int>& connectors) {
    for (const auto connector_id : connectors) {
        this->connectors.at(connector_id)->mutex.unlock();
    }
}

int AuthHandler::select_connector(const std::vector<int>& connectors) {

    if (connectors.size() == 1) {
        return connectors.at(0);
    }

    if (this->selection_algorithm == SelectionAlgorithm::PlugEvents) {
        this->lock_referenced_connectors(connectors);
        if (this->get_latest_plugin(connectors) == -1) {
            this->unlock_referenced_connectors(connectors);
            EVLOG_debug << "No connector in authorization queue. Waiting for a plug in...";
            std::unique_lock<std::mutex> lk(this->auth_queue_mutex);
            if (!this->cv.wait_for(lk, std::chrono::seconds(this->connection_timeout),
                                   [this, connectors] { return this->get_latest_plugin(connectors) != -1; })) {
                return -1;
            }
            this->lock_referenced_connectors(connectors);
            EVLOG_debug << "Plug in at connector occured";
        }
        const auto connector_id = this->get_latest_plugin(connectors);
        return connector_id;
    } else if (this->selection_algorithm == SelectionAlgorithm::UserInput) {
        EVLOG_warning << "SelectionAlgorithm UserInput not yet implemented. Selecting first available connector";
        return connectors.at(0);

    } else {
        throw std::runtime_error("No known SelectionAlgorithm provided: " +
                                 selection_algorithm_to_string(this->selection_algorithm));
    }
}

void AuthHandler::authorize_evse(int connector_id, const Identifier& identifier) {

    const auto evse_index = this->connectors.at(connector_id)->evse_index;

    this->connectors.at(connector_id)->connector.identifier.emplace(identifier);
    this->authorize_callback(evse_index, identifier.id_token);

    std::lock_guard lk(this->timer_mutex);
    this->connectors.at(connector_id)->timeout_timer.stop();
    this->connectors.at(connector_id)
        ->timeout_timer.timeout(
            [this, evse_index]() {
                EVLOG_info << "Authorization timeout for evse#" << evse_index;
                this->withdraw_authorization_callback(evse_index);
            },
            std::chrono::seconds(this->connection_timeout));
    this->plug_in_queue.remove_if([connector_id](int value) { return value == connector_id; });
}

Identifier AuthHandler::get_identifier(const ValidationResult& validation_result, const std::string& id_token) {
    Identifier identifier;
    identifier.id_token = id_token;
    identifier.authorization_status = validation_result.authorization_status;
    identifier.expiry_time = validation_result.expiry_time;
    identifier.parent_id_token = validation_result.parent_id_token;
    return identifier;
}

types::reservation::ReservationResult AuthHandler::handle_reservation(int connector_id,
                                                                      const Reservation& reservation) {
    return this->reservation_handler.reserve(connector_id, this->connectors.at(connector_id)->connector.get_state(),
                                             this->connectors.at(connector_id)->connector.is_reservable, reservation);
}

int AuthHandler::handle_cancel_reservation(int reservation_id) {
    return this->reservation_handler.cancel_reservation(reservation_id);
}

void AuthHandler::call_reserved(const int& connector_id, const int reservation_id) {
    this->reserved_callback(this->connectors.at(connector_id)->evse_index, reservation_id);
}
void AuthHandler::call_reservation_cancelled(const int& connector_id) {
    this->reservation_cancelled_callback(this->connectors.at(connector_id)->evse_index);
}

void AuthHandler::handle_session_event(const int connector_id, const SessionEvent& event) {
    const auto event_type = event.event;
    std::lock_guard lk(this->timer_mutex);
    switch (event_type) {
    case SessionEventEnum::SessionStarted:
        this->connectors.at(connector_id)->connector.is_reservable = false;
        this->plug_in_queue.push_back(connector_id);
        this->cv.notify_one();
        this->connectors.at(connector_id)
            ->timeout_timer.timeout(
                [this, connector_id]() {
                    EVLOG_debug << "Plug In timeout for connector#" << connector_id;
                    this->plug_in_queue.remove_if([connector_id](int value) { return value == connector_id; });
                    this->withdraw_authorization_callback(this->connectors.at(connector_id)->evse_index);
                },
                std::chrono::seconds(this->connection_timeout));
        break;
    case SessionEventEnum::TransactionStarted:
        this->connectors.at(connector_id)->connector.submit_event(Event_Transaction_Started());
        this->connectors.at(connector_id)->timeout_timer.stop();
        break;
    case SessionEventEnum::TransactionFinished:
        this->connectors.at(connector_id)->connector.identifier = boost::none;
        break;
    case SessionEventEnum::SessionFinished:
        this->connectors.at(connector_id)->connector.is_reservable = true;
        this->connectors.at(connector_id)->connector.identifier = boost::none;
        this->connectors.at(connector_id)->connector.submit_event(Event_Session_Finished());
        break;
    case SessionEventEnum::PermanentFault:
        this->connectors.at(connector_id)->connector.submit_event(Event_Faulted());
        break;
    case SessionEventEnum::Error:
        this->connectors.at(connector_id)->connector.submit_event(Event_Faulted());
        break;

    case SessionEventEnum::Disabled:
        this->connectors.at(connector_id)->connector.submit_event(Event_Disable());
        break;

    case SessionEventEnum::Enabled:
        this->connectors.at(connector_id)->connector.submit_event(Event_Enable());
        break;

    case SessionEventEnum::ReservationStart:
        this->connectors.at(connector_id)->connector.reserved = true;
        break;
    case SessionEventEnum::ReservationEnd:
        this->connectors.at(connector_id)->connector.is_reservable = true;
        this->connectors.at(connector_id)->connector.reserved = false;
        break;

    default:
        return;
    }
}

void AuthHandler::set_connection_timeout(const int connection_timeout) {
    this->connection_timeout = connection_timeout;
};

void AuthHandler::set_prioritize_authorization_over_stopping_transaction(bool b) {
    this->prioritize_authorization_over_stopping_transaction = b;
}

void AuthHandler::register_authorize_callback(
    const std::function<void(const int evse_index, const std::string& id_token)>& callback) {
    this->authorize_callback = callback;
}
void AuthHandler::register_withdraw_authorization_callback(const std::function<void(const int evse_index)>& callback) {
    this->withdraw_authorization_callback = callback;
}
void AuthHandler::register_validate_token_callback(
    const std::function<std::vector<ValidationResult>(const std::string& id_token)>& callback) {
    this->validate_token_callback = callback;
}
void AuthHandler::register_stop_transaction_callback(
    const std::function<void(const int evse_index, const StopTransactionReason& reason)>& callback) {
    this->stop_transaction_callback = callback;
}

void AuthHandler::register_reserved_callback(
    const std::function<void(const int& evse_index, const int& reservation_id)>& callback) {
    this->reserved_callback = callback;
}

void AuthHandler::register_reservation_cancelled_callback(const std::function<void(const int& evse_index)>& callback) {
    this->reservation_cancelled_callback = callback;
    this->reservation_handler.register_reservation_cancelled_callback(
        [this](int connector_id) { this->call_reservation_cancelled(connector_id); });
}

} // namespace module