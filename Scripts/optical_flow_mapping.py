# Copyright (c) 2022 YDrive Inc. All rights reserved.

"""
This file contains example code for applying optical flow mapping to an image,
utilizing torch and CUDA.
"""

import argparse

import cv2
import numpy as np
import torch
import torch.nn as nn


def read_image(image_path: str, use_cuda: bool) -> torch.Tensor:
    """
    Reads an image and returns it as a RGB tensor with shape (3, h, w).
    """
    image = cv2.cvtColor(cv2.imread(image_path), cv2.COLOR_BGR2RGB)
    # Convert to the (3, h, w) shape
    image = image.transpose(2, 0, 1)
    tensor = torch.Tensor(image).float()
    if use_cuda:
        tensor = tensor.cuda()
    return tensor


def load_optical_flow(optical_flow_image_path: str, use_cuda: bool) -> torch.Tensor:
    """
    Loads optical flow from an .exr image and returns it as a tensor with shape (2, h, w).
    """
    of_image = cv2.imread(optical_flow_image_path, cv2.IMREAD_ANYCOLOR | cv2.IMREAD_ANYDEPTH)
    h, w, _ = of_image.shape
    # Convert image to HSV, where H is angle and S is intensity
    of_image = cv2.cvtColor(of_image, cv2.COLOR_BGR2HSV)
    ang, mag = of_image[:, :, 0], of_image[:, :, 1]
    dx, dy = cv2.polarToCart(mag, ang, angleInDegrees=True)
    dx = torch.Tensor(-w * dx).unsqueeze(0)
    dy = torch.Tensor(-h * dy).unsqueeze(0)
    flow = torch.cat((dx, dy), dim=0)
    if use_cuda:
        flow = flow.cuda()
    return flow


def map_optical_flow(base_image: torch.Tensor, optical_flow: torch.Tensor) -> torch.Tensor:
    """
    Moves pixels in the base image according to the optical flow.
    Returns the resulting images as a tensor with shape (3, h, w).

    Args:
        base_image      base image tensor with shape (3, h, w)
        optical_flow    optical flow tensor with shape (2, h, w)
    """
    _, h, w = base_image.size()

    # Mesh grid
    xx = torch.arange(0, w).view(1, -1).repeat(h, 1).view(1, h, w)
    yy = torch.arange(0, h).view(-1, 1).repeat(1, w).view(1, h, w)
    grid = torch.cat((xx, yy), 0).float()
    if base_image.is_cuda:
        grid = grid.cuda()

    # Add optical flow movement
    new_grid = grid + optical_flow

    # Scale grid to [-1, 1]
    new_grid[0, :, :] = 2.0 * new_grid[0, :, :] / max(w - 1, 1) - 1.0
    new_grid[1, :, :] = 2.0 * new_grid[1, :, :] / max(h - 1, 1) - 1.0
    new_grid = new_grid.permute(1, 2, 0)

    # The grid sampling method requires higher dimensionality of input tensors,
    # so just add one more dimension to both and remove it on return
    unsqueezed_image = base_image.unsqueeze(0)
    unsqueezed_grid = new_grid.unsqueeze(0)
    output = nn.functional.grid_sample(unsqueezed_image, unsqueezed_grid, mode="bilinear", align_corners=False)
    return output.squeeze()


def store_image(output_image_path: str, tensor: torch.Tensor) -> None:
    """
    Stores an image represented by a tensor.

    Args:
        output_image_path   Path to the output image
        tensor              image tensor with shape (3, h, w)
    """
    if tensor.is_cuda:
        tensor = tensor.cpu()
    image = tensor.detach().numpy()
    # Convert from float to uint8
    image = image.astype(np.uint8)
    # Convert to the (h, w, 3) shape
    image = image.transpose(1, 2, 0)
    cv2.imwrite(output_image_path, cv2.cvtColor(image, cv2.COLOR_BGR2RGB))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'base_image_path',
        type=str,
        help='Path to the base image to apply optical flow to')
    parser.add_argument(
        'optical_flow_image_path',
        type=str,
        help='Path to the optical flow image in the .exr flormat')
    parser.add_argument(
        'output_image_path',
        type=str,
        help='Path to the output image')
    parser.add_argument(
        '--use_cuda',
        '-c',
        action='store_true',
        help='Force using cuda for calculations')
    args = parser.parse_args()

    # Load the base image as tensor
    base_image = read_image(args.base_image_path, args.use_cuda)

    # Load the exr optical flow image as tensor
    flow = load_optical_flow(args.optical_flow_image_path, args.use_cuda)

    # Generate the mapped image
    mapped_image = map_optical_flow(base_image, flow)

    # Store tensor as image
    store_image(args.output_image_path, mapped_image)
