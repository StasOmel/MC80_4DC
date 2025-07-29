# MC80_4DC - Four-Channel DC Motor Controller

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Renesas%20RA8M1-green.svg)](https://www.renesas.com/en/products/ra8m1?utm_campaign=f-up-mcu_ra8m1-epsg-iotbd-ipm1-null&utm_source=null&utm_medium=pr&utm_content=pp#overview)
[![RTOS](https://img.shields.io/badge/RTOS-Azure%20RTOS-orange.svg)](https://azure.microsoft.com/en-us/services/rtos/)
[![Status](https://img.shields.io/badge/status-Experimental-orange.svg)](https://github.com/username/MC80_4DC)
[![Toolchain](https://img.shields.io/badge/toolchain-IAR%209.70-blue.svg)](https://www.iar.com/)
[![Editor](https://img.shields.io/badge/editor-VS%20Code-blue.svg)](https://code.visualstudio.com/)

![MC80_4DC Board](Doc/Img/MC80_3D.png)

An experimental high-performance four-channel DC motor controller based on Renesas RA8M1 microcontroller with advanced motion control algorithms and comprehensive filesystem support.

## 📋 Project Overview

MC80_4DC is an experimental DC motor controller designed for precision motion control applications. The project demonstrates advanced embedded systems programming with real-time operating system integration, high-speed communication interfaces, and motor control with analog position feedback.

**⚠️ Note: This is an experimental project under active development. Some features may be incomplete or subject to change.**

### Key Features

- **Four Independent DC Motor Channels** with individual control
- **Analog Position Feedback**: High-precision analog sensors for position monitoring
- **High-Performance MCU**: Renesas RA8M1 (ARM Cortex-M85, 480MHz)
- **Real-Time Operating System**: Azure RTOS ThreadX integration
- **Storage Solutions**:
  - **exFAT filesystem** on SD card for data logging and configuration
  - **Custom OSPI driver** for external flash memory (under development)
- **Communication Interfaces**: CAN 2.0 for industrial applications
- **Development Environment**: Visual Studio Code for code editing, IAR Embedded Workbench 9.70 for compilation
- **Comprehensive Debug Support**: RTT logging, VT100 terminal interface, SWD/JTAG debug interfaces, and hardware trace

## 🎯 Applications

The MC80_4DC motor controller is designed for a wide range of automation and motion control applications:

- **Multi-leaf Automatic Doors**: Precise control of synchronized door panels
- **3D Rotating Platforms**: Solar panel tracking systems and antenna positioning
- **Automated Blinds and Awnings**: Smart building automation and shading systems
- **Gate and Barrier Control**: Access control systems and parking barriers
- **Stair Lifts**: Mobility assistance and accessibility solutions
- **Industrial Automation**: Various precision motion control applications

## 🛠 Hardware Platform

### Main Controller
- **MCU**: Renesas RA8M1AH3GT (ARM Cortex-M85, 480MHz)
- **Internal Flash**: 2MB
- **External Flash Memory**: MX25UM25645GMI00 (256Mbit OSPI)
- **RAM**: 1MB internal SRAM
- **Package**: LQFP176

### Pinout Diagram
![MC80 Pinout Diagram](Doc/Img/MC80_Pinout_Diagram.png)

### Motor Control System
- **Channels**: 4 independent DC motor drivers
- **Servo Sensors**: 2 analog inputs for servo sensor feedback
- **Quadrature Encoders**: 2 interfaces for incremental position encoders
- **Hall Sensors**: 2 interfaces for 3-phase Hall sensor inputs
- **Analog Inputs**: Equipped with differentiator for low speed measurement
- **Current Monitoring**: Real-time current sensing capabilities
- **PWM Generation**: High-frequency PWM for motor control

### Storage and Communication
- **SD Card**: exFAT filesystem for data storage
- **CAN 2.0**: Industrial communication protocol
- **USB**: Device and host support

### Debug and Programming Interfaces
- **SWD**: Serial Wire Debug interface for programming and debugging
- **Trace**: Hardware trace interface for real-time debugging

## 🎮 Control Methods

The MC80_4DC board supports multiple control interfaces for different operational scenarios:

### Development and Testing
- **USB Control Software**: Specialized control and monitoring software for PC-based testing and debugging
- **Terminal Programs**: Direct control via USB using standard terminal applications
- **Debug Interface**: Full development support through SWD and trace interfaces

![Real-Time Control Application](Doc/Img/MC80_RealTime_Control_App.png)

### Production Operation
- **CAN Bus Control**: Primary control interface for industrial applications
- **Real-time Communication**: Deterministic control through CAN 2.0 protocol
- **Distributed Systems**: Integration into larger automation networks

## 🏗 Software Architecture

### Real-Time Operating System
The project is built on **Azure RTOS ThreadX**, providing:
- Preemptive multitasking
- Real-time scheduling
- Inter-thread communication
- Memory management
- Timer services

### Motor Control Architecture
DC motor control system featuring:
- **Analog Sensor Interface**: Multi-channel ADC for position feedback
- **Current Monitoring**: Real-time current sensing and monitoring
- **PWM Control**: Hardware-generated PWM signals for motor driving

### Filesystem Integration
Advanced storage solution using **exFAT** on SD card:
- FAT32-compatible filesystem
- Long filename support
- Efficient storage management
- Configuration file storage
- Data logging capabilities

### Custom OSPI Driver (Under Development)
High-performance OSPI flash driver featuring:
- Memory-mapped access capabilities
- DMA-based transfers for maximum throughput
- Protocol switching (SPI ↔ Octal DDR)
- Comprehensive error handling
- **Note**: Driver development is ongoing


## 🏃‍♂️ Real-Time Performance

### Task Architecture
- **Motor Control Task**: Real-time motor control operations
- **Sensor Processing Task**: ADC data acquisition and processing
- **Communication Task**: Handle CAN 2.0 communication
- **Filesystem Task**: Background file operations on SD card
- **Monitor Task**: System health monitoring and diagnostics

### Timing Characteristics
- **Sensor Sampling**: High-frequency ADC conversion
- **Interrupt Response**: <5μs average interrupt latency
- **Task Switching**: <1μs context switch time
- **File Operations**: SD card access with optimized buffering


## 📄 License

This project is licensed under the MIT License - see the [`LICENSE`](LICENSE ) file for details.

## � Release Information

### v1.0-filesystem-testing - Filesystem Performance Analysis Complete

This release marks the successful completion of comprehensive filesystem testing and performance analysis for the MC80_4DC platform. The testing framework has been fully validated and provides detailed performance metrics for both supported filesystems.

#### 🎯 Release Highlights

- **Complete Filesystem Testing Framework**: Unified testing interface supporting both LittleFS and FileX filesystems
- **Performance Analysis Tools**: Comprehensive Python-based analysis suite with detailed performance graphs
- **Unified Statistics Engine**: Common performance measurement framework for both filesystems
- **Bug Fixes**: Resolved speed calculation issues and disk space reporting accuracy
- **Configuration Optimization**: FileX directory entry limits configured for optimal performance

#### 📊 Testing Results

The testing framework provides comprehensive analysis of:
- **Read Performance**: File open/close timing and throughput analysis
- **Write Performance**: Write speed analysis with first-file overhead measurements
- **Delete Performance**: Deletion speed analysis with performance degradation tracking
- **Storage Efficiency**: Accurate disk space utilization and percentage calculations

#### 🛠 Technical Improvements

- **Unified Test Functions**: FileX read tests now include pattern validation and size verification
- **Fixed Statistics Calculations**: Corrected average and minimum speed calculations for delete operations
- **Accurate Disk Usage**: Fixed FileX media information display with proper used space calculations
- **Python Analysis Suite**: Created universal log parsers supporting both LittleFS and FileX log formats

#### 📈 Performance Analysis Tools

The release includes Python scripts for detailed performance analysis:
- `Parse_FS_read_log.py` - Read operation analysis with timing breakdowns
- `Parse_FS_write_log.py` - Write operation analysis with speed metrics
- `Parse_FS_delete_log.py` - Delete operation analysis with performance tracking

All analysis tools support both filesystem formats and generate comprehensive matplotlib-based visualizations.

## �🔌 Board Connections

**MC80_4DC** - Experimental DC motor control with precision analog feedback.
![MC80 Connections](Doc/Img/MC80_Connections.png)

---


⚠️ **Disclaimer**: This is an experimental project. Use at your own risk in production environments.
