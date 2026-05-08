# Modern Drive Operating System (MDOS)

MDOS is a lightweight operating system inspired by the classic **DOS**. It aims to provide a fast, command-line-driven environment while maintaining the simplicity of disk operating systems.

## Features

* **Command Line Interface:** A classic prompt-driven environment.
* **Minimalist Core:** Designed for high performance and low overhead.
* **Legacy Inspiration:** Built with the philosophy of vintage 16/32-bit operating systems.

## Getting Started

### Prerequisites
* gnu-efi
* make
* qemu-system-x86_64 (optional for emulating)

### Installation / Compilation
1. **Clone the repository**:
```bash
git clone https://github.com/jasonchristiandev/MDOS.git
```

2. **Build the OS**:
```bash
cd MDOS
make
```

3. **Run the OS using qemu-system-x86_64**:
```bash
make run
```

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Feel free to open an issue or submit a pull request if you find anything lacking.

### How to Contribute

1. **Fork the repository**: Create your own copy of the project to work on.

2. **Create a feature branch**:
```bash
git checkout -b feature/YourFeatureName
```

3. **Commit your changes**: Make sure your code follows our style, or atleast similar to our style.

4. **Push and pull request**: Push your branch to GitHub and open a Pull Request. Please provide a clear description of what your code adds or fixes.