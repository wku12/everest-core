#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/generated.rs"));

use generated::{get_config, Module, OnReadySubscriber, ModulePublisher, types::authorization::ProvidedIdToken, AuthTokenProviderServicePublisher, AuthTokenProviderServiceSubscriber};

use payment_terminal::config::{Config, FeigConfig};
use proto::payment_terminal::v1::{CardTypeEnum};
use std::{net::Ipv4Addr, str::FromStr};

use std::sync::{Arc, Mutex};
use tokio::runtime::Builder;
use anyhow::{Result, bail};
use tokio::time::{sleep, Duration, Instant};
use log::{info, warn, debug};

pub struct OneClass {
    token_provider_callback: Mutex<Option<AuthTokenProviderServicePublisher>>,
}

impl AuthTokenProviderServiceSubscriber for OneClass {}

impl OnReadySubscriber for OneClass {
    fn on_ready(&self, publishers: &ModulePublisher) {
        *self.token_provider_callback.lock().unwrap() = Some(publishers.token_provider.clone());
    }
}


fn main() -> Result<()> {
    info!("Hello!");
    let config = get_config();
    info!("Received the config {config:?}");
    let temp = OneClass{token_provider_callback: Mutex::new(None)};

    embedded_logger::init_logger("PAYMENT_TERMINAL_LOGGER_LEVEL");
    let pt_config = Config {
        terminal_id: config.terminal_id,
        feig_serial: config.feig_serial,
        ip_address: Ipv4Addr::from_str(&config.ip)?,
        feig_config: FeigConfig{currency: config.currency as usize, pre_authorization_amount: config.pre_authorization_amount as usize},
    };
    let one_class = Arc::new(temp);

    let _module = Module::new(
        one_class.clone(),
        one_class.clone(),
    );
    let rt = Arc::new(Builder::new_multi_thread().enable_all().build().unwrap());
    rt.block_on(async {
        // TODO: This should be in the on_ready actually
        let mut feig = payment_terminal::feig::Feig::new(pt_config).await?;
        let s = sleep(Duration::from_secs(5));
        tokio::pin!(s);
        loop {
            info!("Ready to read card!");
            let response = feig.read_card().await;

            match response {
                Ok(card_info) => {
                    match card_info.card_type() {
                        CardTypeEnum::CardTypeBank => {
                            warn!("Received bank card. Not handling this yet");
                        }
                        CardTypeEnum::CardTypeMembership => {
                            let token = ProvidedIdToken {
                                id_token: card_info.tag_id.unwrap(),
                                authorization_type: generated::types::authorization::AuthorizationType::RFID,
                                certificate: Option::None,
                                connectors: Option::None,
                                id_token_type: Option::None,
                                iso_15118_certificate_hash_data: Option::None,
                                prevalidated: Option::None,
                                request_id: None,
                            };
                            debug!("{:?}", token);
                            match one_class.token_provider_callback.lock().unwrap().as_ref() {
                                Some(m) => {
                                    println!("Sending token!");
                                    m.provided_token(token).unwrap();
                                }
                                None => {}
                            }
                        }
                    }
                    tokio::select! {
                        () = &mut s => {
                            debug!("Timer elapsed");
                            s.as_mut().reset(Instant::now() + Duration::from_secs(5));
                        },
                    }
                }
                Err(e) => match e.downcast_ref::<payment_terminal::feig::Error>() {
                    Some(payment_terminal::feig::Error::NoCardPresented) => {
                        // Ignore this error
                        debug!("No card presented");
                    }
                    _ => {
                        warn!("Bailing {:?}", e);
                        bail!("Failed {:?}", e);
                    }
                },
            }
        }
    })
}

