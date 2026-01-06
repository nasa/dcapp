#!/usr/bin/env python3
"""
Generate Lissajous curve sample files for dcapp.

This creates:
- lissajous.xml: The display definition with Line vertices referencing variables
- logic/logic.c: C code that calculates vertex positions each frame
- logic/dcapp.h: Auto-generated header for variable access

The Lissajous curve formula:
  X = A * sin(a * t + delta)
  Y = B * sin(b * t)

By animating delta, the curve morphs beautifully over time.
"""

import os
import math

# Configuration
NUM_POINTS = 200          # Number of vertices in the line
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 800
VIRTUAL_WIDTH = 800
VIRTUAL_HEIGHT = 800
CENTER_X = VIRTUAL_WIDTH / 2
CENTER_Y = VIRTUAL_HEIGHT / 2
AMPLITUDE = 350           # How big the curve is

# Lissajous parameters (a:b ratio determines the pattern)
# Common beautiful ratios: 3:2, 3:4, 5:4, 5:6
A_FREQ = 3
B_FREQ = 4

def generate_xml():
    """Generate the XML display file."""
    lines = []
    lines.append('<?xml version="1.0"?>')
    lines.append('<DCAPP>')
    lines.append('')
    lines.append('    <!-- Phase variable animated by logic file -->')
    lines.append('    <Variable Type="#_variable_double_" InitialValue="0">PHASE</Variable>')
    lines.append('')
    lines.append('    <!-- Vertex position variables -->')

    for i in range(NUM_POINTS):
        lines.append(f'    <Variable Type="#_variable_double_" InitialValue="0">X{i}</Variable>')
        lines.append(f'    <Variable Type="#_variable_double_" InitialValue="0">Y{i}</Variable>')

    lines.append('')
    lines.append('    <Logic File="logic/logic.so"/>')
    lines.append('')
    lines.append(f'    <Window Title="Lissajous Curve" X="30" Y="30" Width="{WINDOW_WIDTH}" Height="{WINDOW_HEIGHT}">')
    lines.append('        <Default>')
    lines.append('            <Text Size="14" FillColor="1 1 1"/>')
    lines.append('        </Default>')
    lines.append('')
    lines.append(f'        <Panel VirtualWidth="{VIRTUAL_WIDTH}" VirtualHeight="{VIRTUAL_HEIGHT}" BackgroundColor="0.05 0.05 0.08">')
    lines.append('')
    lines.append('            <!-- Title -->')
    lines.append(f'            <Text X="{CENTER_X}" Y="{VIRTUAL_HEIGHT - 30}" LocalAlignX="#_align_center_" Size="20" FillColor="0.6 0.8 1">Lissajous Curve ({A_FREQ}:{B_FREQ})</Text>')
    lines.append('')
    lines.append('            <!-- The animated curve -->')
    lines.append('            <Line LineColor="0.2 0.8 1" LineWidth="2">')

    for i in range(NUM_POINTS):
        lines.append(f'                <Vertex X="@X{i}" Y="@Y{i}"/>')

    lines.append('            </Line>')
    lines.append('')
    lines.append('            <!-- Second curve with phase offset for visual interest -->')
    lines.append('            <Line LineColor="1 0.4 0.6 0.5" LineWidth="1.5">')

    # We'll create a second set of variables for a trailing curve
    for i in range(NUM_POINTS):
        lines.append(f'                <Vertex X="@X{i}" Y="@Y{i}"/>')

    lines.append('            </Line>')
    lines.append('')
    lines.append('            <!-- Phase display -->')
    lines.append(f'            <Text X="{VIRTUAL_WIDTH - 20}" Y="20" LocalAlignX="#_align_right_" Size="12" FillColor="0.5 0.5 0.5">Phase: @PHASE(%.2f)</Text>')
    lines.append('')
    lines.append('        </Panel>')
    lines.append('    </Window>')
    lines.append('</DCAPP>')

    return '\n'.join(lines)


def generate_logic_c():
    """Generate the logic C file."""
    lines = []
    lines.append('#include <math.h>')
    lines.append('#include "dcapp.h"')
    lines.append('')
    lines.append(f'#define NUM_POINTS {NUM_POINTS}')
    lines.append(f'#define CENTER_X {CENTER_X}')
    lines.append(f'#define CENTER_Y {CENTER_Y}')
    lines.append(f'#define AMPLITUDE {AMPLITUDE}')
    lines.append(f'#define A_FREQ {A_FREQ}')
    lines.append(f'#define B_FREQ {B_FREQ}')
    lines.append('#define PI 3.14159265358979323846')
    lines.append('#define TWO_PI (2.0 * PI)')
    lines.append('')
    lines.append('// Pointers to vertex variables')
    lines.append('static double *x_vars[NUM_POINTS];')
    lines.append('static double *y_vars[NUM_POINTS];')
    lines.append('')
    lines.append('void display_init(void) {')
    lines.append('    // Store pointers to vertex variables for fast access')

    for i in range(NUM_POINTS):
        lines.append(f'    x_vars[{i}] = X{i};')
        lines.append(f'    y_vars[{i}] = Y{i};')

    lines.append('}')
    lines.append('')
    lines.append('void display_draw(void) {')
    lines.append('    // Increment phase for animation')
    lines.append('    *PHASE += 0.02;')
    lines.append('    if (*PHASE > TWO_PI) {')
    lines.append('        *PHASE -= TWO_PI;')
    lines.append('    }')
    lines.append('')
    lines.append('    // Calculate vertex positions')
    lines.append('    for (int i = 0; i < NUM_POINTS; i++) {')
    lines.append('        double t = (double)i / (double)(NUM_POINTS - 1) * TWO_PI;')
    lines.append('        ')
    lines.append('        // Lissajous formula with animated phase')
    lines.append('        double x = AMPLITUDE * sin(A_FREQ * t + *PHASE);')
    lines.append('        double y = AMPLITUDE * sin(B_FREQ * t);')
    lines.append('        ')
    lines.append('        // Translate to center of panel')
    lines.append('        *x_vars[i] = CENTER_X + x;')
    lines.append('        *y_vars[i] = CENTER_Y + y;')
    lines.append('    }')
    lines.append('}')
    lines.append('')
    lines.append('void display_close(void) {')
    lines.append('}')

    return '\n'.join(lines)


def generate_dcapp_h():
    """Generate the dcapp.h header file."""
    lines = []
    lines.append('// ********************************************* //')
    lines.append('// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //')
    lines.append('// ********************************************* //')
    lines.append('')
    lines.append('#include <stdbool.h>')
    lines.append('')
    lines.append('#ifndef _DCAPP_LOGIC_EXTERN_')
    lines.append('#define _DCAPP_LOGIC_EXTERN_')
    lines.append('')
    lines.append('double *PHASE;')

    for i in range(NUM_POINTS):
        lines.append(f'double *X{i};')
        lines.append(f'double *Y{i};')

    lines.append('')
    lines.append('#ifdef __cplusplus')
    lines.append('extern "C" {')
    lines.append('#endif')
    lines.append('')
    lines.append('void display_init();')
    lines.append('void display_draw();')
    lines.append('void display_close();')
    lines.append('')
    lines.append('typedef void *(*_GetVariableValueAddr)(const char *name);')
    lines.append('void display_pre_init(_GetVariableValueAddr get_variable_value_addr) {')
    lines.append('    if (get_variable_value_addr) {')
    lines.append('        PHASE = (double *)get_variable_value_addr("PHASE");')

    for i in range(NUM_POINTS):
        lines.append(f'        X{i} = (double *)get_variable_value_addr("X{i}");')
        lines.append(f'        Y{i} = (double *)get_variable_value_addr("Y{i}");')

    lines.append('    }')
    lines.append('}')
    lines.append('')
    lines.append('#ifdef __cplusplus')
    lines.append('}')
    lines.append('#endif')
    lines.append('')
    lines.append('#else')
    lines.append('')
    lines.append('extern double *PHASE;')

    for i in range(NUM_POINTS):
        lines.append(f'extern double *X{i};')
        lines.append(f'extern double *Y{i};')

    lines.append('')
    lines.append('#endif')

    return '\n'.join(lines)


def generate_makefile():
    """Generate a Makefile for building the logic shared library."""
    lines = []
    lines.append('CC = gcc')
    lines.append('CFLAGS = -fPIC -shared -O2')
    lines.append('')
    lines.append('all: logic.so')
    lines.append('')
    lines.append('logic.so: logic.c dcapp.h')
    lines.append('\t$(CC) $(CFLAGS) -o logic.so logic.c -lm')
    lines.append('')
    lines.append('clean:')
    lines.append('\trm -f logic.so')
    lines.append('')
    lines.append('.PHONY: all clean')

    return '\n'.join(lines)


def main():
    # Create directories
    script_dir = os.path.dirname(os.path.abspath(__file__))
    logic_dir = os.path.join(script_dir, 'logic')
    os.makedirs(logic_dir, exist_ok=True)

    # Generate files
    xml_path = os.path.join(script_dir, 'lissajous.xml')
    logic_c_path = os.path.join(logic_dir, 'logic.c')
    dcapp_h_path = os.path.join(logic_dir, 'dcapp.h')
    makefile_path = os.path.join(logic_dir, 'Makefile')

    with open(xml_path, 'w') as f:
        f.write(generate_xml())
    print(f'Generated: {xml_path}')

    with open(logic_c_path, 'w') as f:
        f.write(generate_logic_c())
    print(f'Generated: {logic_c_path}')

    with open(dcapp_h_path, 'w') as f:
        f.write(generate_dcapp_h())
    print(f'Generated: {dcapp_h_path}')

    with open(makefile_path, 'w') as f:
        f.write(generate_makefile())
    print(f'Generated: {makefile_path}')

    print(f'\nTo build and run:')
    print(f'  cd {logic_dir} && make')
    print(f'  ./out/dcapp samples/lissajous/lissajous.xml')


if __name__ == '__main__':
    main()
