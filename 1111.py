import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# 光伏电站参数（示例：北纬30°，东经120°，装机容量5MW，转换效率20%）
LAT = 30.0       # 纬度（°）
LONG = 120.0     # 经度（°）
P0 = 5e6         # 装机容量（W）
ETA = 0.20       # 组件转换效率
G_STC = 1000     # 标准测试辐照度（W/m²）
RHO = 0.75       # 大气透明度系数（经验值）
G0 = 1367        # 太阳常数（W/m²）

def calculate_sun_position(doy, hour):
    """计算太阳赤纬和时角"""
    delta = 23.45 * np.sin(np.radians(360 * (doy - 81) / 365))  # 太阳赤纬（°）
    omega = (hour - 12) * 15  # 时角（°）
    return delta, omega

def calculate_irradiance(lat, delta, omega):
    """计算太阳高度角和辐照度"""
    alpha = np.arcsin(
        np.sin(np.radians(lat)) * np.sin(np.radians(delta)) +
        np.cos(np.radians(lat)) * np.cos(np.radians(delta)) * np.cos(np.radians(omega))
    )
    alpha_deg = np.degrees(alpha)
    if alpha_deg < 0:  # 太阳低于地平线时辐照度为0
        return 0.0, 0.0
    m = 1 / np.cos(alpha) if alpha > 0 else 0  # 大气质量
    Gt = G0 * np.cos(alpha) * (RHO ** m)  # 地面辐照度（W/m²）
    return Gt, alpha_deg
