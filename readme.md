# Dylatte
Exploring self decrypting code at runtime on macOS

## Why?
An interesting approach to hardening natively compiled software is run time self-modifying binaries. Specifically where encryption is deployed. This approach allows engineers to raise the bar in terms of knowledge required to reverse engineer the software, as well as hide any methods used to further harden the software, such as integrity checks and anti-debugging measures.

For example, a developer may want to employ encryption so that they can initiate integrity checks, and remove the code used to do so from the runtime memory, making it much more complicated to reverse engineer, of course, this still has its drawbacks, as with most software hardening techniques, they work best when deployed together.

# Interested? 

Read more about it [here](https://chris.tech/exploring-self-decrypting-binaries-on-macos)