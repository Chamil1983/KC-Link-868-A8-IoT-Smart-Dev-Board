# Contributing to KC-Link PRO A8

Thank you for your interest in contributing to the KC-Link PRO A8 project! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Process](#development-process)
- [Pull Request Process](#pull-request-process)
- [Coding Guidelines](#coding-guidelines)
- [Documentation](#documentation)
- [Testing](#testing)
- [Issues and Bug Reports](#issues-and-bug-reports)
- [Feature Requests](#feature-requests)

## Code of Conduct

This project adheres to a Code of Conduct that all contributors are expected to follow. Please read [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) for details.

## Getting Started

1. **Fork the repository** on GitHub.
2. **Clone your fork** to your local machine:
   ```bash
   git clone https://github.com/your-username/kc-link-pro-a8.git
   cd kc-link-pro-a8
   ```
3. **Set up the development environment**:
   - Install Arduino IDE (version 1.8.5 or later)
   - Install ESP32 board support
   - Install required libraries (PCF8574, OneWire, DallasTemperature, DHT, etc.)
4. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Process

1. **Check the Issues** tab for open issues or create a new issue to discuss your proposed changes.
2. **Discuss with the maintainers** before making significant changes.
3. **Work on your changes** in your feature branch.
4. **Test your changes** thoroughly with actual hardware if possible.
5. **Submit a Pull Request** when ready.

## Pull Request Process

1. **Update documentation** to reflect any changes you've made.
2. **Include appropriate tests** for your changes.
3. **Update the README.md** if necessary.
4. **Submit your Pull Request** with a clear title and description.
5. **Respond to feedback** from reviewers.

Your PR will be reviewed by the maintainers who may request changes or provide feedback. Once approved, your changes will be merged into the main branch.

## Coding Guidelines

### Arduino Code Style

- Use 2-space indentation.
- Keep lines under 80 characters whenever possible.
- Use camelCase for variable and function names.
- Use PascalCase for class names.
- Prefix private member variables with an underscore (e.g., `_privateVar`).
- Comment your code to explain complex logic.
- Include Doxygen-style function documentation:

```cpp
/**
 * Sets the state of a relay
 * @param relayNumber The relay to control (1-8)
 * @param state The desired state (true=ON, false=OFF)
 * @return true if successful, false otherwise
 */
bool setRelay(int relayNumber, bool state);
```

### C++ Guidelines

- Follow the [Arduino Style Guide](https://www.arduino.cc/en/Reference/StyleGuide) for Arduino-specific code.
- For other C++ code, follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
- Avoid using dynamic memory allocation if possible; if necessary, manage memory carefully.
- Keep functions focused on a single task.
- Make error handling robust and informative.

## Documentation

Good documentation is critical to this project:

- Document all public functions with clear descriptions, parameters, and return values.
- Keep the README.md up to date.
- Include comments in your code.
- Add examples for new features.
- Update the API Reference documentation.
- Include wiring diagrams or schematics for hardware configurations.

## Testing

- Test your changes with actual KC-Link PRO A8 hardware when possible.
- Write unit tests for new functionality.
- Test compatibility with various versions of the Arduino IDE.
- Verify compatibility with other libraries that might be used alongside this one.
- Test with different versions of the ESP32 board package.

## Issues and Bug Reports

When creating an issue for a bug, please include:

- A clear description of the bug
- Steps to reproduce
- Expected behavior
- Actual behavior
- Hardware setup (version of the board, connections, etc.)
- Software environment (Arduino IDE version, ESP32 board package version, etc.)
- Screenshots or code snippets if applicable
- Any error messages

Use the bug report template provided in the repository.

## Feature Requests

When proposing a new feature:

- Clearly describe the feature and its use case
- Explain why it would be valuable to the project
- Indicate if you're willing to implement it yourself
- Consider how it might affect existing functionality

Use the feature request template provided in the repository.

## Communication

- Use GitHub Issues for bug reports, feature requests, and discussions.
- Contact the maintainers directly for security issues.
- Join our community forum for general discussions.

## License

By contributing to this project, you agree that your contributions will be licensed under the same [MIT License](LICENSE) that covers the project.

---

Thank you for contributing to the KC-Link PRO A8 project! Your efforts help make this project better for everyone.
