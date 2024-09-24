#include "ocpp_conversions.hpp"

#include "generated/types/display_message.hpp"

namespace ocpp_conversions {
types::display_message::MessagePriorityEnum
to_everest_display_message_priority(const ocpp::v201::MessagePriorityEnum& priority) {
    switch (priority) {
    case ocpp::v201::MessagePriorityEnum::AlwaysFront:
        return types::display_message::MessagePriorityEnum::AlwaysFront;
    case ocpp::v201::MessagePriorityEnum::InFront:
        return types::display_message::MessagePriorityEnum::InFront;
    case ocpp::v201::MessagePriorityEnum::NormalCycle:
        return types::display_message::MessagePriorityEnum::NormalCycle;
    default:
        throw std::out_of_range(
            "Could not convert ocpp::v201::MessagePriorityEnum to types::display_message::MessagePriorityEnum");
    }
}

ocpp::v201::MessagePriorityEnum
to_ocpp_201_message_priority(const types::display_message::MessagePriorityEnum& priority) {
    switch (priority) {
    case types::display_message::MessagePriorityEnum::AlwaysFront:
        return ocpp::v201::MessagePriorityEnum::AlwaysFront;
    case types::display_message::MessagePriorityEnum::InFront:
        return ocpp::v201::MessagePriorityEnum::InFront;
    case types::display_message::MessagePriorityEnum::NormalCycle:
        return ocpp::v201::MessagePriorityEnum::NormalCycle;
    default:
        throw std::out_of_range(
            "Could not convert types::display_message::MessagePriorityEnum to ocpp::v201::MessagePriorityEnum");
    }
}

types::display_message::MessageStateEnum to_everest_display_message_state(const ocpp::v201::MessageStateEnum& state) {
    switch (state) {
    case ocpp::v201::MessageStateEnum::Charging:
        return types::display_message::MessageStateEnum::Charging;
    case ocpp::v201::MessageStateEnum::Faulted:
        return types::display_message::MessageStateEnum::Faulted;
    case ocpp::v201::MessageStateEnum::Idle:
        return types::display_message::MessageStateEnum::Idle;
    case ocpp::v201::MessageStateEnum::Unavailable:
        return types::display_message::MessageStateEnum::Unavailable;
    default:
        throw std::out_of_range(
            "Could not convert ocpp::v201::MessageStateEnum to types::display_message::MessageStateEnum");
    }
}

ocpp::v201::MessageStateEnum to_ocpp_201_display_message_state(const types::display_message::MessageStateEnum& state) {
    switch (state) {
    case types::display_message::MessageStateEnum::Charging:
        return ocpp::v201::MessageStateEnum::Charging;
    case types::display_message::MessageStateEnum::Faulted:
        return ocpp::v201::MessageStateEnum::Faulted;
    case types::display_message::MessageStateEnum::Idle:
        return ocpp::v201::MessageStateEnum::Idle;
    case types::display_message::MessageStateEnum::Unavailable:
        return ocpp::v201::MessageStateEnum::Unavailable;
    default:
        throw std::out_of_range(
            "Could not convert types::display_message::MessageStateEnum to ocpp::v201::MessageStateEnum");
    }
}

types::display_message::MessageFormat
to_everest_display_message_format(const ocpp::v201::MessageFormatEnum& message_format) {
    switch (message_format) {
    case ocpp::v201::MessageFormatEnum::ASCII:
        return types::display_message::MessageFormat::ASCII;
    case ocpp::v201::MessageFormatEnum::HTML:
        return types::display_message::MessageFormat::HTML;
    case ocpp::v201::MessageFormatEnum::URI:
        return types::display_message::MessageFormat::URI;
    case ocpp::v201::MessageFormatEnum::UTF8:
        return types::display_message::MessageFormat::UTF8;
    default:
        throw std::out_of_range(
            "Could not convert ocpp::v201::MessageFormat to types::display_message::MessageFormatEnum");
    }
}

ocpp::v201::MessageFormatEnum to_ocpp_201_message_format_enum(const types::display_message::MessageFormat& format) {
    switch (format) {
    case types::display_message::MessageFormat::ASCII:
        return ocpp::v201::MessageFormatEnum::ASCII;
    case types::display_message::MessageFormat::HTML:
        return ocpp::v201::MessageFormatEnum::HTML;
    case types::display_message::MessageFormat::URI:
        return ocpp::v201::MessageFormatEnum::URI;
    case types::display_message::MessageFormat::UTF8:
        return ocpp::v201::MessageFormatEnum::UTF8;
    }

    throw std::out_of_range("Could not convert types::display_message::MessageFormat to ocpp::v201::MessageFormatEnum");
}

ocpp::IdentifierType to_ocpp_identifiertype_enum(const types::display_message::Identifier_type identifier_type) {
    switch (identifier_type) {
    case types::display_message::Identifier_type::IdToken:
        return ocpp::IdentifierType::IdToken;
    case types::display_message::Identifier_type::SessionId:
        return ocpp::IdentifierType::SessionId;
    case types::display_message::Identifier_type::TransactionId:
        return ocpp::IdentifierType::TransactionId;
    }

    throw std::out_of_range("types::display_message::Identifier_type to ocpp::IdentierType");
}

types::display_message::Identifier_type to_everest_identifier_type_enum(const ocpp::IdentifierType identifier_type) {
    switch (identifier_type) {
    case ocpp::IdentifierType::SessionId:
        return types::display_message::Identifier_type::SessionId;
    case ocpp::IdentifierType::IdToken:
        return types::display_message::Identifier_type::IdToken;
    case ocpp::IdentifierType::TransactionId:
        return types::display_message::Identifier_type::TransactionId;
    }
    throw std::out_of_range("ocpp::IdentierType to types::display_message::Identifier_type");
}

types::display_message::MessageContent
to_everest_display_message_content(const ocpp::DisplayMessageContent& message_content) {
    types::display_message::MessageContent message;
    message.content = message_content.message;
    if (message_content.message_format.has_value()) {
        message.format = to_everest_display_message_format(message_content.message_format.value());
    }
    message.language = message_content.language;

    return message;
}

types::display_message::DisplayMessage to_everest_display_message(const ocpp::DisplayMessage& display_message) {
    types::display_message::DisplayMessage m;
    m.id = display_message.id;
    m.message.content = display_message.message.message;
    if (display_message.message.message_format.has_value()) {
        m.message.format = to_everest_display_message_format(display_message.message.message_format.value());
    }
    m.message.language = display_message.message.language;

    if (display_message.priority.has_value()) {
        m.priority = ocpp_conversions::to_everest_display_message_priority(display_message.priority.value());
    }

    m.qr_code = display_message.qr_code;
    m.identifier_id = display_message.identifier_id;
    if (display_message.identifier_type.has_value()) {
        m.identifier_type = to_everest_identifier_type_enum(display_message.identifier_type.value());
    }

    if (display_message.state.has_value()) {
        m.state = to_everest_display_message_state(display_message.state.value());
    }

    if (display_message.timestamp_from.has_value()) {
        m.timestamp_from = display_message.timestamp_from.value().to_rfc3339();
    }

    if (display_message.timestamp_to.has_value()) {
        m.timestamp_to = display_message.timestamp_to.value().to_rfc3339();
    }

    return m;
}

ocpp::DisplayMessage to_ocpp_display_message(const types::display_message::DisplayMessage& display_message) {
    ocpp::DisplayMessage m;
    m.id = display_message.id;
    m.message.message = display_message.message.content;
    m.message.language = display_message.message.language;
    if (display_message.message.format.has_value()) {
        m.message.message_format = to_ocpp_201_message_format_enum(display_message.message.format.value());
    }

    if (display_message.priority.has_value()) {
        m.priority = ocpp_conversions::to_ocpp_201_message_priority(display_message.priority.value());
    }

    m.qr_code = display_message.qr_code;
    m.identifier_id = display_message.identifier_id;
    if (display_message.identifier_type.has_value()) {
        m.identifier_type = to_ocpp_identifiertype_enum(display_message.identifier_type.value());
    }

    if (display_message.state.has_value()) {
        m.state = to_ocpp_201_display_message_state(display_message.state.value());
    }

    m.timestamp_from = display_message.timestamp_from;
    m.timestamp_to = display_message.timestamp_to;

    return m;
}

types::session_cost::SessionStatus to_everest_running_cost_state(const ocpp::RunningCostState& state) {
    switch (state) {
    case ocpp::RunningCostState::Charging:
        return types::session_cost::SessionStatus::Running;
    case ocpp::RunningCostState::Idle:
        return types::session_cost::SessionStatus::Idle;
    case ocpp::RunningCostState::Finished:
        return types::session_cost::SessionStatus::Finished;
    default:
        throw std::out_of_range("Could not convert ocpp::RunningCostState to types::session_cost::SessionStatus");
    }
}

types::session_cost::SessionCostChunk create_session_cost_chunk(const double& price, const uint32_t& number_of_decimals,
                                                                const std::optional<ocpp::DateTime>& timestamp,
                                                                const std::optional<uint32_t>& meter_value) {
    types::session_cost::SessionCostChunk chunk;
    chunk.cost = types::money::MoneyAmount();
    chunk.cost->value = static_cast<int>(price * (pow(10, number_of_decimals)));
    if (timestamp.has_value()) {
        chunk.timestamp_to = timestamp.value().to_rfc3339();
    }
    chunk.metervalue_to = meter_value;
    return chunk;
}

types::money::Price create_price(const double& price, const uint32_t& number_of_decimals,
                                 std::optional<types::money::CurrencyCode> currency_code) {
    types::money::Price p;
    types::money::Currency currency;
    currency.code = currency_code;
    currency.decimals = number_of_decimals;
    p.currency = currency;
    p.value.value = static_cast<int>(price * (pow(10, number_of_decimals)));
    return p;
}

types::session_cost::ChargingPriceComponent
create_charging_price_component(const double& price, const uint32_t& number_of_decimals,
                                const types::session_cost::CostCategory category,
                                std::optional<types::money::CurrencyCode> currency_code) {
    types::session_cost::ChargingPriceComponent c;
    c.category = category;
    c.price = create_price(price, number_of_decimals, currency_code);
    return c;
}

types::session_cost::SessionCost create_session_cost(const ocpp::RunningCost& running_cost,
                                                     const uint32_t number_of_decimals,
                                                     std::optional<types::money::CurrencyCode> currency_code) {
    types::session_cost::SessionCost cost;
    cost.session_id = running_cost.transaction_id;
    cost.currency.code = currency_code;
    cost.currency.decimals = static_cast<int32_t>(number_of_decimals);
    cost.status = to_everest_running_cost_state(running_cost.state);
    cost.qr_code = running_cost.qr_code_text;
    if (running_cost.cost_messages.has_value()) {
        cost.message = std::vector<types::display_message::MessageContent>();
        for (const ocpp::DisplayMessageContent& message : running_cost.cost_messages.value()) {
            types::display_message::MessageContent m = to_everest_display_message_content(message);
            cost.message->push_back(m);
        }
    }

    types::session_cost::SessionCostChunk chunk = create_session_cost_chunk(
        running_cost.cost, number_of_decimals, running_cost.timestamp, running_cost.meter_value);
    cost.cost_chunks = std::vector<types::session_cost::SessionCostChunk>();
    cost.cost_chunks->push_back(chunk);

    if (running_cost.charging_price.has_value()) {
        cost.charging_price = std::vector<types::session_cost::ChargingPriceComponent>();
        const ocpp::RunningCostChargingPrice& price = running_cost.charging_price.value();
        if (price.hour_price.has_value()) {
            types::session_cost::ChargingPriceComponent hour_price = create_charging_price_component(
                price.hour_price.value(), number_of_decimals, types::session_cost::CostCategory::Time, currency_code);
            cost.charging_price->push_back(hour_price);
        }
        if (price.kWh_price.has_value()) {
            types::session_cost::ChargingPriceComponent energy_price = create_charging_price_component(
                price.kWh_price.value(), number_of_decimals, types::session_cost::CostCategory::Energy, currency_code);
            cost.charging_price->push_back(energy_price);
        }
        if (price.flat_fee.has_value()) {
            types::session_cost::ChargingPriceComponent flat_fee_price = create_charging_price_component(
                price.flat_fee.value(), number_of_decimals, types::session_cost::CostCategory::FlatFee, currency_code);
            cost.charging_price->push_back(flat_fee_price);
        }
    }

    if (running_cost.idle_price.has_value()) {
        types::session_cost::IdlePrice idle_price;
        const ocpp::RunningCostIdlePrice& ocpp_idle_price = running_cost.idle_price.value();
        if (ocpp_idle_price.idle_hour_price.has_value()) {
            idle_price.hour_price =
                create_price(ocpp_idle_price.idle_hour_price.value(), number_of_decimals, currency_code);
        }

        if (ocpp_idle_price.idle_grace_minutes.has_value()) {
            idle_price.grace_minutes = ocpp_idle_price.idle_grace_minutes.value();
        }

        cost.idle_price = idle_price;
    }

    if (running_cost.next_period_at_time.has_value() || running_cost.next_period_charging_price.has_value() ||
        running_cost.next_period_idle_price.has_value()) {
        types::session_cost::NextPeriodPrice next_period;
        if (running_cost.next_period_at_time.has_value()) {
            next_period.timestamp_from = running_cost.next_period_at_time.value().to_rfc3339();
        }
        if (running_cost.next_period_idle_price.has_value()) {
            types::session_cost::IdlePrice next_period_idle_price;
            const ocpp::RunningCostIdlePrice& ocpp_next_period_idle_price = running_cost.next_period_idle_price.value();
            if (ocpp_next_period_idle_price.idle_hour_price.has_value()) {
                next_period_idle_price.hour_price = create_price(ocpp_next_period_idle_price.idle_hour_price.value(),
                                                                 number_of_decimals, currency_code);
            }

            if (ocpp_next_period_idle_price.idle_grace_minutes.has_value()) {
                next_period_idle_price.grace_minutes = ocpp_next_period_idle_price.idle_grace_minutes.value();
            }

            next_period.idle_price = next_period_idle_price;
        }
        if (running_cost.next_period_charging_price.has_value()) {
            const ocpp::RunningCostChargingPrice& next_period_charging_price =
                running_cost.next_period_charging_price.value();

            next_period.charging_price = std::vector<types::session_cost::ChargingPriceComponent>();

            if (next_period_charging_price.hour_price.has_value()) {
                types::session_cost::ChargingPriceComponent hour_price =
                    create_charging_price_component(next_period_charging_price.hour_price.value(), number_of_decimals,
                                                    types::session_cost::CostCategory::Time, currency_code);
                next_period.charging_price.push_back(hour_price);
            }

            if (next_period_charging_price.kWh_price.has_value()) {
                types::session_cost::ChargingPriceComponent energy_price =
                    create_charging_price_component(next_period_charging_price.kWh_price.value(), number_of_decimals,
                                                    types::session_cost::CostCategory::Energy, currency_code);
                next_period.charging_price.push_back(energy_price);
            }

            if (next_period_charging_price.flat_fee.has_value()) {
                types::session_cost::ChargingPriceComponent flat_fee_price =
                    create_charging_price_component(next_period_charging_price.flat_fee.value(), number_of_decimals,
                                                    types::session_cost::CostCategory::FlatFee, currency_code);
                next_period.charging_price.push_back(flat_fee_price);
            }
        }

        cost.next_period = next_period;
    }

    return cost;
}
} // namespace ocpp_conversions