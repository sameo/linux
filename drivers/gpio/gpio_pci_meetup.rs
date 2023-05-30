// Copyright © 2023 Rivos
//
// SPDX-License-Identifier: GPL-2.0
//

//! PCI GPIO Expander

use kernel::prelude::*;
use kernel::{
    bindings, bit, c_str, define_pci_id_table, device, driver,
    gpio::{self, LineDirection, Registration},
    io_mem::IoMem,
    pci,
    sync::Arc,
    types::ForeignOwnable,
};

const N_GPIOS: u16 = 32;
const GPIO_SIZE: usize = 0x100;
const GPIO_PCI_BAR: usize = 2;
const GPIO_DIR: usize = 0x0000;

struct GpioPciRegistration {
    gpio_chip: Pin<Box<Registration<GpioPciDevice>>>,
}

struct GpioPciResource {
    base: IoMem<GPIO_SIZE>,
}

type GpioPciDeviceData = device::Data<GpioPciRegistration, GpioPciResource, ()>;

struct GpioPciDevice;

#[vtable]
impl gpio::Chip for GpioPciDevice {
    type Data = Arc<GpioPciDeviceData>;

    fn get_direction(
        data: <Self::Data as ForeignOwnable>::Borrowed<'_>,
        offset: u32,
    ) -> Result<LineDirection> {
        let gpio_chip = data.resources().ok_or(ENXIO)?;
        Ok(if gpio_chip.base.readq(GPIO_DIR) & bit(offset) != 0 {
            LineDirection::Out
        } else {
            LineDirection::In
        })
    }

    fn direction_input(
        _data: <Self::Data as ForeignOwnable>::Borrowed<'_>,
        _offset: u32,
    ) -> Result {
        Ok(())
    }

    fn direction_output(
        _data: <Self::Data as ForeignOwnable>::Borrowed<'_>,
        _offset: u32,
        _value: bool,
    ) -> Result {
        Ok(())
    }

    fn get(_data: <Self::Data as ForeignOwnable>::Borrowed<'_>, _offset: u32) -> Result<bool> {
        Ok(true)
    }

    fn set(_data: <Self::Data as ForeignOwnable>::Borrowed<'_>, _offset: u32, _value: bool) {}
}

impl pci::Driver for GpioPciDevice {
    type Data = Arc<GpioPciDeviceData>;

    define_pci_id_table! {
    (),
        [
            // GPIO PCI Expander
            (pci::DeviceId::new(0x494F, 0x0DC8), None),
        ]
    }

    fn probe(dev: &mut pci::Device, _id: Option<&Self::IdInfo>) -> Result<Arc<GpioPciDeviceData>> {
        pr_info!("GPIO PCI probe\n");

        dev.enable_device_mem()?;
        dev.set_master();
        let bars = dev.select_bars(bindings::IORESOURCE_MEM.into());
        // TODO: Release resources on failure.
        dev.request_selected_regions(bars, c_str!("gpio"))?;
        let res = dev.take_resource(GPIO_PCI_BAR).ok_or(ENXIO)?;
        let bar = unsafe { IoMem::<GPIO_SIZE>::try_new(res) }?;

        let gpio_chip = Pin::from(Box::try_new(Registration::<GpioPciDevice>::new())?);
        let pci_data = Pin::from(kernel::new_device_data!(
            GpioPciRegistration { gpio_chip },
            GpioPciResource { base: bar },
            (),
            "GpioPciDevice::Data"
        )?);

        let gpio_pci_data = Arc::<GpioPciDeviceData>::from(pci_data);
        kernel::gpio_chip_register!(
            gpio_pci_data
                .clone()
                .registrations()
                .ok_or(ENXIO)?
                .gpio_chip
                .as_mut(),
            N_GPIOS,
            None,
            dev,
            gpio_pci_data.clone()
        )?;

        pr_info!("GPIO PCI probe succeeded!\n");
        Ok(gpio_pci_data)
    }

    fn remove(_data: &Self::Data) {
        todo!()
    }
}

struct MeetupModule {
    _registration: Pin<Box<driver::Registration<pci::Adapter<GpioPciDevice>>>>,
}

impl kernel::Module for MeetupModule {
    fn init(_name: &'static CStr, module: &'static ThisModule) -> Result<Self> {
        Ok(Self {
            _registration: driver::Registration::new_pinned(
                c_str!("PCI GPIO Expander (meetup)"),
                module,
            )?,
        })
    }
}
module! {
    type: MeetupModule,
    name: "rustmeetup",
    author: "Samuel Ortiz",
    license: "GPL v2",
}
