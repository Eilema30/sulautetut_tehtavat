
*** Settings ***
Library    SerialLibrary    encoding=ascii

*** Variables ***
${PORT}       COM5

*** Test Cases ***
Test Correct Time
    Add Port    ${PORT}    baudrate=115200    timeout=3
    Write Data    000005
    Write Data    ${\n}
    ${response}=    Read Until    terminator=0A    terminator_encoding=hexlify
    Log To Console    ${response}
    Should Contain    ${response}    5
    Delete All Ports

Test Zero Time
    Add Port    ${PORT}    baudrate=115200    timeout=3
    Write Data    000000
    Write Data    ${\n}
    ${response}=    Read Until    terminator=0A    terminator_encoding=hexlify
    Log To Console    ${response}
    Should Contain    ${response}    0
    Delete All Ports

Test Invalid Minutes
    Add Port    ${PORT}    baudrate=115200    timeout=3
    Write Data    006000
    Write Data    ${\n}
    ${response}=    Read Until    terminator=0A    terminator_encoding=hexlify
    Log To Console    ${response}
    Should Contain    ${response}    -3
    Delete All Ports

Test Invalid Hours
    Add Port    ${PORT}    baudrate=115200    timeout=3
    Write Data    250000
    Write Data    ${\n}
    ${response}=    Read Until    terminator=0A    terminator_encoding=hexlify
    Log To Console    ${response}
    Should Contain    ${response}    -3
    Delete All Ports

Test Invalid Characters
    Add Port    ${PORT}    baudrate=115200    timeout=3
    Write Data    abcdef
    Write Data    ${\n}
    ${response}=    Read Until    terminator=0A    terminator_encoding=hexlify
    Log To Console    ${response}
    Should Contain    ${response}    -3
    Delete All Ports
