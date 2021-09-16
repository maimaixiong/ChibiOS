# CAN Gateway

## Project
ChibiOS/demos/STM32/RT-STM32F407-CGW


## Openpilot for VW

- volkswagen/values.py
-
-   CANBUS:
-     pt = 0  #J533/Gateway Side , PowerTrain CAN
-     cam = 2 #CAR  Side ,  Extended CAN or CAM/RADAR CAN

    DBC_FILE:
        vw_mqb_2010.dbc

- volkswagen/carcontroller.py

    ACC Button Controls and GRA

- LH_EPS_03/0x9f from Powertrain to CAM/RADAR CAN? 

- HCA_01/0x126(create_mqb_steering_control) from CAM CAN to Powertrain CAN

- LDW_02/0x397(create_mqb_hud_control) from CAM CAN to Powertrain CAN


def create_mqb_hud_control(packer, bus, enabled, steering_pressed, hud_alert, left_lane_visible, right_lane_visible,
                           ldw_stock_values, left_lane_depart, right_lane_depart):
  # Lane color reference:
  # 0 (LKAS disabled) - off
  # 1 (LKAS enabled, no lane detected) - dark gray
  # 2 (LKAS enabled, lane detected) - light gray on VW, green or white on Audi depending on year or virtual cockpit.  On a color MFD on a 2015 A3 TDI it is white, virtual cockpit on a 2018 A3 e-Tron its green.
  # 3 (LKAS enabled, lane departure detected) - white on VW, red on Audi
  values = ldw_stock_values.copy()
  values.update({
    "LDW_Status_LED_gelb": 1 if enabled and steering_pressed else 0,         #黄色，LDW使能，但用户转方向盘
    "LDW_Status_LED_gruen": 1 if enabled and not steering_pressed else 0,    #绿色，LDW使能，自动转向
    "LDW_Lernmodus_links": 3 if left_lane_depart else 1 + left_lane_visible,
    "LDW_Lernmodus_rechts": 3 if right_lane_depart else 1 + right_lane_visible,
    "LDW_Texte": hud_alert,
  })
  return packer.make_can_msg("LDW_02", bus, values)

        # sig_name, sig_address, default
        ("LDW_SW_Warnung_links", "LDW_02", 0),      # Blind spot in warning mode on left side due to lane departure
        ("LDW_SW_Warnung_rechts", "LDW_02", 0),     # Blind spot in warning mode on right side due to lane departure
        ("LDW_Seite_DLCTLC", "LDW_02", 0),          # Direction of most likely lane departure (left or right)
        ("LDW_DLC", "LDW_02", 0),                   # Lane departure, distance to line crossing
        ("LDW_TLC", "LDW_02", 0),                   # Lane departure, time to line crossing
      ]

- GRA_ACC_01/0x12B(create_mqb_acc_buttons_control) from Powertrain to CAM/RADAR CAN

    # Read ACC hardware button type configuration info that has to pass thru
    # to the radar. Ends up being different for steering wheel buttons vs
    # third stalk type controls.
    # RADAR需要读PT总线的硬件按钮类型的配置信息，轮式按键和杆式控制的类型最终会不同

    self.graHauptschalter = pt_cp.vl["GRA_ACC_01"]["GRA_Hauptschalter"]
      ("GRA_Hauptschalter", "GRA_ACC_01", 0),       # ACC button, on/off
    self.graTypHauptschalter = pt_cp.vl["GRA_ACC_01"]["GRA_Typ_Hauptschalter"]
      ("GRA_Typ_Hauptschalter", "GRA_ACC_01", 0),   # ACC main button type
    self.graButtonTypeInfo = pt_cp.vl["GRA_ACC_01"]["GRA_ButtonTypeInfo"]
      ("GRA_ButtonTypeInfo", "GRA_ACC_01", 0),      # unknown related to stalk type
    self.graTipStufe2 = pt_cp.vl["GRA_ACC_01"]["GRA_Tip_Stufe_2"]
      ("GRA_Tip_Stufe_2", "GRA_ACC_01", 0),         # unknown related to stalk type

      ("GRA_Abbrechen", "GRA_ACC_01", 0),           # ACC button, cancel
      ("GRA_Tip_Setzen", "GRA_ACC_01", 0),          # ACC button, set
      ("GRA_Tip_Hoch", "GRA_ACC_01", 0),            # ACC button, increase or accel
      ("GRA_Tip_Runter", "GRA_ACC_01", 0),          # ACC button, decrease or decel
      ("GRA_Tip_Wiederaufnahme", "GRA_ACC_01", 0),  # ACC button, resume
      ("GRA_Verstellung_Zeitluecke", "GRA_ACC_01", 0),  # ACC button, time gap adj
      


