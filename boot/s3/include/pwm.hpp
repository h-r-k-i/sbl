#ifndef PWM_HPP
#define PWM_HPP

/*******************************************************************************************
 * 
 * FILENAME:    ./boot/s3/include/pwm.hpp
 * NAME:        SBL, Stage 3, Power Management || Header
 * DESCRIPTION: Power management functions for the SBL Stage 3 bootloader.
 * AUTHOR:      Haruki Tokumei, Haruki Media Group
 * DATE:        13 November 2026
 * COPYRIGHT:   Haruki Media Group.
 * LICENSE:     MIT License (./LICENSE)
 *******************************************************************************************/

extern "C" void ShutDownPC();
// Waits one second
extern "C" void LiteralWait();

#endif // PWM_HPP