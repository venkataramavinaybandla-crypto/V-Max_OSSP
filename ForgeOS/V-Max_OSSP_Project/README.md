# Linux System Information and Resource Monitoring Tool

A C-based Linux terminal application developed to monitor important system resources and display useful system information in a simple and organized way.

## About the Project

This project provides a menu-driven interface for checking different aspects of a Linux system. Instead of using multiple commands separately, the application brings basic system monitoring features together in one program.

The project uses Linux system interfaces and the `/proc` filesystem to collect information about CPU, memory, processes, disk storage, and system uptime.

## Features

- System Information
  - Operating system
  - Kernel version
  - System architecture
  - Hostname
  - CPU information
  - Current user information

- CPU Monitoring
  - CPU utilization percentage
  - Live CPU usage monitoring

- Memory Monitoring
  - Total, used, and available RAM
  - Cached and buffer memory
  - Swap memory usage
  - Memory usage status

- Disk Monitoring
  - Total, used, and free disk space
  - Disk usage percentage
  - Inode information

- Process Monitoring
  - List of running processes
  - Process ID and parent process ID
  - Process state
  - Process tree
  - Process statistics

- System Uptime
  - System running time
  - Days, hours, minutes, and seconds
  - Total CPU time since boot

## Technologies Used

- C Programming
- Linux / Ubuntu
- GCC Compiler
- Linux `/proc` Filesystem
- Linux System Calls and APIs

## How It Works

The application provides a main menu from which the user can select the required monitoring option.

System information is collected using Linux system calls such as `uname()`, while resource information is read from files available through the `/proc` filesystem. Disk information is obtained using Linux filesystem APIs.

The collected data is then processed and displayed in the terminal in a readable format.
