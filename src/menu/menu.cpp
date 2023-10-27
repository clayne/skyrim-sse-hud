#include <event/BSTCrosshairRefEvent.h>
#include <event/BSTMenuEvent.h>
#include <fonts/IconsMaterialDesignIcons.h>
#include <hook/hudhook.h>
#include <imgui/imgui.h>
#include <memory/autotake.h>
#include <memory/lotd.h>
#include <memory/memory.h>
#include <memory/npc.h>
#include <menu/lotd.h>
#include <menu/menu.h>
#include <menu/theme.h>
#include <nlohmann/json.hpp>
#include <setting/setting.h>
#include <utils/GeneralUtil.h>
#include <utils/NameUtil.h>

namespace menu
{
	int trackX1 = 0;
	int trackY1 = 0;
	int trackX2 = 0;
	int trackY2 = 0;

	RE::NiPointer<RE::NiCamera> getCamera()
	{
		RE::PlayerCamera* camera = RE::PlayerCamera::GetSingleton();
		if (camera->cameraRoot->children.size() == 0)
			return nullptr;
		for (auto& entry : camera->cameraRoot->children) {
			auto asCamera = skyrim_cast<RE::NiCamera*>(entry.get());
			if (asCamera)
				return RE::NiPointer<RE::NiCamera>(asCamera);
		}
		return nullptr;
	};

	bool auto_remove_ammo = false;
	bool remember_last_ammo = false;
	int hotkey = 0;
	int hotkey2 = 3;
	float font_scale = 1.0f;

	// 默认配置
	bool show_player_base_info_window_sep = false;
	bool flag_process = false;
	bool flag_base_info_setting = true;
	bool show_player_debug_window = false;
	bool no_titlebar = true;
	bool no_resize = false;
	bool no_collapse = false;
	bool no_background = false;
	bool no_background_items = false;
	bool auto_resize = true;

	// 20220428 追加
	bool window_border = true;
	bool frame_border = true;
	bool bullet_text = true;

	//static int refresh_time_show = 1000;
	float colorPlotHistogramX = 0.9f;
	float colorPlotHistogramY = 0.7f;
	float colorPlotHistogramZ = 0;
	float colorPlotHistogramW = 1;

	//static bool show_npc_window_dis = false;
	//static int show_npc_window_dis_meter = 30;
	extern bool show_npc_window_formid = false;
	static int show_inv_window_height = 15;
	static float show_inv_window_height_half = 6.7f;
	static bool show_inv_window_active = true;
	static bool show_crosshair = false;

	// 基础属性进度条
	static float progress = 0;
	static float progress2 = 0;
	static float progress3 = 0;
	static int stateMod_lastSelectIndex = -1;
	static int stateMod_selectIndex = 0;
	static float stateMod_nowValue = 0;
	static float stateMod_newValue = 0;
	static int statePerMod_lastSelectIndex = -1;
	static int statePerMod_selectIndex = 0;
	static float statePerMod_nowValue = 0;
	static float statePerMod_newValue = 0;

	static bool setvalue_Permanent = false;
	static int getInv_nowValue = 100;
	static int getInv_selectIndex = 0;

	ImVec4 colorProgress(1.0f, 0.5f, 0.0f, 1.0f);

	const char* actorValues[] = {
		"负重 [kCarryWeight]",
		"生命恢复 [kHealRate]",
		"魔法恢复 [kMagickaRate]",
		"耐力恢复 [KStaminaRate]",
		"行走速度 [kSpeedMult]",
		"武器基础伤害 [kMeleeDamage]",
		"空手基础伤害 [kUnarmedDamage]"
	};
	RE::ActorValue actorValuesIndex[] = {
		RE::ActorValue::kCarryWeight,
		RE::ActorValue::kHealRate,
		RE::ActorValue::kMagickaRate,
		RE::ActorValue::KStaminaRate,
		RE::ActorValue::kSpeedMult,
		RE::ActorValue::kMeleeDamage,
		RE::ActorValue::kUnarmedDamage,
	};

	const char* perActorValues[] = {
		"毒抗 [kPoisonResist]",
		"火抗 [kResistFire]",
		"冰抗 [kResistFrost]",
		"雷抗 [kResistShock]",
		"法抗 [kResistMagic]",
		"抗疾病 [kResistDisease]",
		"远程基础伤害 [kMarksmanPowerModifier]",
		"法术基础伤害 [kDestructionPowerModifier]"
	};
	RE::ActorValue perActorValuesIndex[] = {
		RE::ActorValue::kPoisonResist,
		RE::ActorValue::kResistFire,
		RE::ActorValue::kResistFrost,
		RE::ActorValue::kResistShock,
		RE::ActorValue::kResistMagic,
		RE::ActorValue::kResistDisease,
		RE::ActorValue::kMarksmanPowerModifier,
		RE::ActorValue::kDestructionPowerModifier,
	};

	const char* invValues[] = {
		"铁锭 [5ACE4]",
		"钢锭 [5ACE5]",
		"黑檀锭 [5AD9D]",
		"皮带 [800E4]"
	};

	const char* invValuesCode[] = {
		"5ACE4",
		"5ACE5",
		"5AD9D",
		"800E4"
	};

	std::vector<RE::Actor*> actors;

	enum TableColumn
	{
		TableColumn_1,
		TableColumn_2,
		TableColumn_3,
	};
	enum PlayerInfoColumnID
	{
		PlayerInfoColumnID_ID,
		PlayerInfoColumnID_1,
		PlayerInfoColumnID_2,
		PlayerInfoColumnID_3,
		PlayerInfoColumnID_4,
		PlayerInfoColumnID_5,
		PlayerInfoColumnID_6,
		PlayerInfoColumnID_7,
		PlayerInfoColumnID_8,
		PlayerInfoColumnID_9

	};

	void myTextColored(const ImVec4 col, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextColoredV(col, fmt, args);
		va_end(args);
	}

	bool show_npc = true;
	bool show_enemy = true;
	bool show_teammate = true;
	bool show_horse = true;

	void __fastcall buildPlayerInvInfo(int count, InventoryInfo inv[])
	{
		static ImGuiTableFlags flags =
			/*	ImGuiTableFlags_Reorderable
			| ImGuiTableFlags_Hideable*/
			ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoBordersInBody
			/*	| ImGuiTableFlags_NoBordersInBody
				| ImGuiTableFlags_ScrollX
				| ImGuiTableFlags_SizingFixedFit*/
			;

		const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
		if (ImGui::BeginTable("table_sorting", 6, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * show_inv_window_height), 0.0f)) {
			ImGui::TableSetupColumn("已装备", ImGuiTableColumnFlags_WidthFixed, 40.0f, PlayerInfoColumnID_ID);
			ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 100.0f, PlayerInfoColumnID_1);
			ImGui::TableSetupColumn("数量", ImGuiTableColumnFlags_WidthFixed, 40.0f, PlayerInfoColumnID_2);
			ImGui::TableSetupColumn("重量", ImGuiTableColumnFlags_WidthFixed, 40, PlayerInfoColumnID_3);
			ImGui::TableSetupColumn("FORMID", ImGuiTableColumnFlags_WidthFixed, 100.0f, PlayerInfoColumnID_4);
			//ImGui::TableSetupColumn("地址", ImGuiTableColumnFlags_WidthFixed, 0.0f, PlayerInfoColumnID_5);
			ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
			ImGui::TableHeadersRow();

			ImGuiListClipper clipper;
			clipper.Begin(count);
			while (clipper.Step())
				for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
					// Display a data item
					//InventoryInfo* item = getPlayerInvData(row_n);
					InventoryInfo& item = inv[row_n];
					//if (item) {
					ImGui::PushID(row_n + 7000);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("%s", item.isWorn ? "×" : "");
					ImGui::TableNextColumn();

					if (ImGui::Selectable(item.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
						//if (ImGui::IsMouseDoubleClicked(0)) {
						if (item.ptr->IsAmmo() || item.ptr->IsArmor() || item.ptr->IsWeapon()) {
							RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
							auto actorEquipManager = RE::ActorEquipManager::GetSingleton();
							if (item.isWorn) {
								actorEquipManager->UnequipObject(player, item.ptr, item.invExtraPtr, 1);
								item.isWorn = false;
							} else {
								actorEquipManager->EquipObject(player, item.ptr, item.invExtraPtr, 1);
								item.isWorn = true;
							}
						}
						//}
					}
					ImGui::TableNextColumn();
					//if (isShowFriend && isShowEnemy) {
					ImGui::Text("%d", item.count);
					//}
					ImGui::TableNextColumn();
					//if (isAim) {
					ImGui::Text("%.1f", item.weight);
					//}
					ImGui::TableNextColumn();
					ImGui::Text("%s", item.formIdStr.c_str());
					//ImGui::TableNextColumn();
					//ImGui::Text("0x%X", item->address);
					ImGui::PopID();
					//}
				}
			ImGui::EndTable();
		}
	}

	void __fastcall buildNpcInfo(int active2, ActorInfo* actor2Info, int actorCount)
	{
		if (active2) {
			for (int i = 0; i < actorCount; i++) {
				auto& item = actor2Info[i];

				char hp[16] = "已死亡";
				if (item.lifeState != RE::ACTOR_LIFE_STATE::kDead) {
					snprintf(hp, 16, "%.0f/%.0f", item.kHealth, item.kHealthBase);
				}

				bool treeNodeExResult;
				if (show_npc_window_formid) {
					treeNodeExResult = ImGui::TreeNodeEx(item.formIdStr.c_str(), 0, "%s - [%d] %s [ %s ]", item.formIdStr.c_str(), item.level, item.name.c_str(), hp);
				} else {
					treeNodeExResult = ImGui::TreeNodeEx(item.formIdStr.c_str(), 0, "[%d] %s [ %s ]", item.level, item.name.c_str(), hp);
				}

				if (treeNodeExResult) {
					ImGui::PushID(i + 1000);
					if (ImGui::SmallButton("\uf101 传送玩家")) {
						std::string commandStr = "player.moveto ";
						commandStr.append(item.formIdStr);
						ScriptUtil::ExecuteCommand(commandStr);
					}
					ImGui::PopID();

					if (item.lifeState != RE::ACTOR_LIFE_STATE::kDead) {
						ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
						ImGui::PushID(i + 2000);
						if (ImGui::SmallButton("\uf100 传送目标")) {
							std::string commandStr = "moveto player";
							//commandStr.append(item.formId);
							//commandStr.append("moveto player");

							// 调用控制台
							const auto scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
							const auto script = scriptFactory ? scriptFactory->Create() : nullptr;
							if (script) {
								script->SetCommand(commandStr);
								script->CompileAndRun(item.ptr);
								delete script;
							}
						}
						ImGui::PopID();
					}

					if (trackActorPtrs.find(item.ptr) == trackActorPtrs.end()) {
						auto player = RE::PlayerCharacter::GetSingleton();
						if (item.ptr->GetCurrentLocation() == player->currentLocation) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::PushID(i + 3000);
							if (ImGui::SmallButton(ICON_MDI_MAP_MARKER_RADIUS " 位置标记")) {
								trackActorPtrs.insert(item.ptr);
							}
							ImGui::PopID();
						}
					}

					if (item.inventoryCount > 0) {
						ImGui::Separator();
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
						for (int i2 = 0; i2 < item.inventoryCount; i2++) {
							auto& inv = item.Inventorys[i2];

							char buf[80];
							if (inv.count > 1) {
								if (show_npc_window_formid) {
									snprintf(buf, 80, "%s %s - %s %s (%d) %.1f ", u8"\uf01c", inv.formIdStr.c_str(), inv.isWorn ? "[装备中]" : "", inv.name.c_str(), inv.count, inv.weight);
								} else {
									snprintf(buf, 80, "%s %s %s (%d) %.1f ", u8"\uf01c", inv.isWorn ? "[装备中]" : "", inv.name.c_str(), inv.count, inv.weight);
								}
							} else {
								if (show_npc_window_formid) {
									snprintf(buf, 80, "%s %s - %s %s %.1f ", u8"\uf01c", inv.formIdStr.c_str(), inv.isWorn ? "[装备中]" : "", inv.name.c_str(), inv.weight);
								} else {
									snprintf(buf, 80, "%s %s %s %.1f ", u8"\uf01c", inv.isWorn ? "[装备中]" : "", inv.name.c_str(), inv.weight);
								}
							}
							if (ImGui::Selectable(buf, false)) {
								auto player = RE::PlayerCharacter::GetSingleton();
								item.ptr->RemoveItem(inv.ptr, 1, RE::ITEM_REMOVE_REASON::kSelling, 0, player);
							}
						}
						ImGui::PopStyleColor();
					}

					ImGui::TreePop();
				}
			}
		} else {
			for (int i = 0; i < actorCount; i++) {
				auto& item = actor2Info[i];

				if (item.isInCombat) {
					myTextColored(ImVec4(1, 1, 0, 1), "\uf071");

				} else {
					ImGui::Text("   ");
				}
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
				ImGui::Text("[%d] %s [", item.level, item.name.c_str());
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

				if (item.lifeState == RE::ACTOR_LIFE_STATE::kDead) {
					myTextColored(ImVec4(1, 0, 0.0f, 1.0f), "已死亡");
				} else {
					float enemyHealthRate = item.kHealth / (item.kHealthBase == 0 ? 1 : item.kHealthBase);
					if (enemyHealthRate > 0.85f) {
						if (item.kHealthBase == item.kHealth) {
							myTextColored(ImVec4(0.0f, 1, 0.0f, 1.0f), "%.0f/%.0f", item.kHealth, item.kHealthBase);
						} else {
							myTextColored(ImVec4(0.0f, 1, 0.0f, 1.0f), "%.1f/%.0f", item.kHealth, item.kHealthBase);
						}
					} else if (enemyHealthRate < 0.20f) {
						myTextColored(ImVec4(1, 0.5f, 0.0f, 1.0f), "%.1f/%.0f", item.kHealth, item.kHealthBase);
					} else if (enemyHealthRate <= 0) {
						myTextColored(ImVec4(1, 0, 0.0f, 1.0f), "0/%.0f", item.kHealthBase);
					} else {
						ImGui::Text("%.1f/%.0f", item.kHealth, item.kHealthBase);
					}
				}

				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
				ImGui::Text("]");

				if (show_npc_window_direction) {
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					switch (item.direction) {
					case 1:  //前
						ImGui::Text(" \uf062%.0f", item.distance);
						break;
					case 2:  //左
						ImGui::Text(" \uf060%.0f", item.distance);
						break;
					case 3:
						ImGui::Text(" \uf063%.0f", item.distance);
						break;
					case 4:
						ImGui::Text(" \uf061%.0f", item.distance);
						break;
					default:
						ImGui::Text(" %.0f", item.distance);
						break;
					}
				}
			}
		}
	}

	void __fastcall buildItemCONTInfo(int count, ItemInfoCONT* items, RE::FormType formType)
	{
		static ImGuiTableFlags flagsItem =
			ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;

		const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		int columnCount = 4;

		if (show_items_window_direction) {
			columnCount++;
		}
		if (show_items_window_refid) {
			columnCount++;
		}
		if (show_items_window_formid) {
			columnCount++;
		}
		columnCount++;

		if (show_items_window_file) {
			columnCount++;
		}

		float tableHeight = TEXT_BASE_HEIGHT * show_inv_window_height;
		if (show_items_window_settings && show_items_window_auto_cont) {
			if (show_items_window_auto_conttype) {
				tableHeight = TEXT_BASE_HEIGHT * (show_inv_window_height_half + 1);
			} else {
				tableHeight = TEXT_BASE_HEIGHT * 13.5;
			}
		}

		if (ImGui::BeginTable("tableItemCONT", columnCount, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 15, tableHeight), 0.0f)) {
			ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 60 * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_1);
			ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_2);
			ImGui::TableSetupColumn("数量", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_3);

			if (show_items_window_direction) {
				ImGui::TableSetupColumn("方位", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_7);
			}

			ImGui::TableSetupColumn("拥有者", ImGuiTableColumnFlags_WidthFixed , 70.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_9);
			if (show_items_window_refid) {
				ImGui::TableSetupColumn("REFID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_8);
			}
			if (show_items_window_formid) {
				ImGui::TableSetupColumn("FORMID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_4);
			}
			if (show_items_window_file) {
				ImGui::TableSetupColumn("MOD", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_6);
			}

			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_5);

			/*if (show_items_window_settings) {
			}*/

			ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
			ImGui::TableHeadersRow();

			for (int row_n = 0; row_n < count; row_n++) {
				ItemInfoCONT& item = items[row_n];
				ImGui::PushID(item.formId + 0x1000000);
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (item.invCount > 0) {
					bool openFlag;
					if (item.isCrime) {
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
						openFlag = ImGui::TreeNode(item.formIdStr.c_str(), "%s", item.name.c_str());
					} else {
						if (show_items_window_auto_conttype) {
							if (item.isAuto) {
								openFlag = ImGui::TreeNode(item.formIdStr.c_str(), "%s" ICON_MDI_AUTORENEW, item.name.c_str());
							} else {
								openFlag = ImGui::TreeNode(item.formIdStr.c_str(), "%s", item.name.c_str());
							}
						} else {
							openFlag = ImGui::TreeNode(item.formIdStr.c_str(), "%s" ICON_MDI_AUTORENEW, item.name.c_str());
						}
					}

					if (item.isCrime) {
						ImGui::PopStyleColor();
					} else {
					}

					if (openFlag) {
						for (int i2 = 0; i2 < item.invCount; i2++) {
							auto& inv = item.invs[i2];
							char buf[80];

							if (inv.count > 1) {
								snprintf(buf, 80, item.isEnchanted ? "%s" ICON_MDI_FLASH " (%d)" : "%s (%d)", inv.name.c_str(), inv.count);
							} else {
								snprintf(buf, 80, item.isEnchanted ? "%s" ICON_MDI_FLASH : "%s", inv.name.c_str());
							}

							if (inv.isCrime) {
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
								if (ImGui::Selectable(buf, false)) {
									/*auto player = RE::PlayerCharacter::GetSingleton();
									item.ptr->RemoveItem(inv.ptr, inv.count, RE::ITEM_REMOVE_REASON::kSteal, 0, player);*/

									RemoveItemCONT(nullptr, item.ptr, inv.ptr, inv.count, false);
								}
								ImGui::PopStyleColor();
							} else {
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
								if (ImGui::Selectable(buf, false)) {
									/*auto player = RE::PlayerCharacter::GetSingleton();
									item.ptr->RemoveItem(inv.ptr, inv.count, RE::ITEM_REMOVE_REASON::kRemove, 0, player);*/

									RemoveItemCONT(nullptr, item.ptr, inv.ptr, inv.count, false);
								}
								ImGui::PopStyleColor();
							}
						}
						ImGui::TreePop();
					}

				} else {
					myTextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s[空]", item.name.c_str());
				}

				ImGui::TableNextColumn();
				switch (item.lockLevel) {
				case RE::LOCK_LEVEL::kVeryEasy:
					ImGui::Text("新手锁");
					break;
				case RE::LOCK_LEVEL::kEasy:
					ImGui::Text("熟练锁");
					break;
				case RE::LOCK_LEVEL::kAverage:
					ImGui::Text("老手锁");
					break;
				case RE::LOCK_LEVEL::kHard:
					ImGui::Text("专家锁");
					break;
				case RE::LOCK_LEVEL::kVeryHard:
					ImGui::Text("大师锁");
					break;
				case RE::LOCK_LEVEL::kRequiresKey:
					ImGui::Text("加密锁");
					break;
				case RE::LOCK_LEVEL::kUnlocked:
				default:
					ImGui::Text("-");
					break;
				}

				ImGui::TableNextColumn();
				ImGui::Text("%d", item.invCount);

				if (show_items_window_direction) {
					ImGui::TableNextColumn();
					switch (item.direction) {
					case 1:  //前
						ImGui::Text(" \uf062%.0f", item.distance);
						break;
					case 2:  //左
						ImGui::Text(" \uf060%.0f", item.distance);
						break;
					case 3:
						ImGui::Text(" \uf063%.0f", item.distance);
						break;
					case 4:
						ImGui::Text(" \uf061%.0f", item.distance);
						break;
					default:
						ImGui::Text(" %.0f", item.distance);
						break;
					}
				}

				ImGui::TableNextColumn();
				ImGui::Text("%s", item.ownerName.c_str());

				if (show_items_window_refid) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", item.formIdStr.c_str());
				}

				if (show_items_window_formid) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", item.baseFormIdStr.c_str());
				}

				if (show_items_window_file) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", item.filename.c_str());
				}

				ImGui::TableNextColumn();
				if (show_items_window_settings) {
					if (ImGui::SmallButton("\uf101")) {
						std::string commandStr = "player.moveto ";
						commandStr.append(item.formIdStr);
						ScriptUtil::ExecuteCommand(commandStr);
						if (activeItems) {
							activeItems = false;
						}
					}

					if (show_items_window_auto_cont && show_items_window_auto_conttype) {
						if (!item.isAuto) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_AUTORENEW)) {
								bool exist = false;
								for (const auto& includeForm : autoContForms) {
									if (includeForm.formId == item.baseFormId) {
										exist = true;
										break;
									}
								}
								if (!exist) {
									autoContForms.push_back({ item.baseFormId, item.name, item.formTypeStr });
								}
								autoContFormIds.insert(item.baseFormId);
							}
						}
					}
				}

				if (trackPtrs.find(item.ptr) == trackPtrs.end()) {
					if (show_items_window_settings) {
						ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					}
					if (ImGui::SmallButton(ICON_MDI_MAP_MARKER_RADIUS)) {
						trackPtrs.insert(item.ptr);
					}
				}

				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		if (show_items_window_settings && show_items_window_auto_cont) {
			if (show_items_window_auto_conttype) {
				float tableHeight2 = TEXT_BASE_HEIGHT * (show_inv_window_height_half - 1);
				ImGui::Checkbox(ICON_MDI_AUTORENEW " 容器类型过滤(点击" ICON_MDI_AUTORENEW "按钮添加拾取白名单)", &show_items_window_auto_conttype);

				int columnCount2 = 3;
				if (ImGui::BeginTable("tableItemCONTauto", columnCount2, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 15, tableHeight2), 0.0f)) {
					ImGui::TableSetupColumn("FORMID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, TableColumn_1);
					ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 60.0f * ImGui::GetIO().FontGlobalScale, TableColumn_2);
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40.0f * ImGui::GetIO().FontGlobalScale, TableColumn_3);

					ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
					ImGui::TableHeadersRow();

					int deleteFormId = 0;

					ImGuiListClipper clipper;
					clipper.Begin(autoContForms.size());
					while (clipper.Step())
						for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
							IncludeForm& item = autoContForms[row_n];
							ImGui::PushID(item.formId + 0x2000000);
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("%08X", item.formId);
							ImGui::TableNextColumn();
							ImGui::Text("%s", item.name.c_str());
							ImGui::TableNextColumn();
							//ImGui::Text("%s", item.formTypeStr.c_str());

							if (ImGui::SmallButton(ICON_MDI_CLOSE)) {
								deleteFormId = item.formId;
							}

							ImGui::PopID();
						}
					ImGui::EndTable();
					if (deleteFormId) {
						autoContFormIds.erase(deleteFormId);
						autoContForms.erase(std::remove_if(autoContForms.begin(), autoContForms.end(),
												[&deleteFormId](const IncludeForm& x) {  // 这种位置写错后报错很难追溯，做个标记以后找解决方法
													return x.formId == deleteFormId;
												}),
							autoContForms.end());
					}
				}
			} else {
				ImGui::Checkbox(ICON_MDI_AUTORENEW " 容器类型过滤", &show_items_window_auto_conttype);
			}
		}
	}

	void __fastcall buildItemACHRInfo(int count, ItemInfoCONT* items, RE::FormType formType)
	{
		static ImGuiTableFlags flagsItem =
			ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;

		const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		int columnCount = 2;

		if (show_items_window_direction) {
			columnCount++;
		}
		if (show_items_window_formid) {
			columnCount++;
		}
		columnCount++;

		if (ImGui::BeginTable("tableItemACHR", columnCount, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 15, TEXT_BASE_HEIGHT * show_inv_window_height), 0.0f)) {
			ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_1);
			//ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed, 40.0f, PlayerInfoColumnID_2);
			ImGui::TableSetupColumn("数量", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_3);

			if (show_items_window_direction) {
				ImGui::TableSetupColumn("方位", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_7);
			}
			if (show_items_window_formid) {
				ImGui::TableSetupColumn("FORMID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_4);
			}

			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_5);
			/*if (show_items_window_settings) {
			}*/

			ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
			ImGui::TableHeadersRow();

			for (int row_n = 0; row_n < count; row_n++) {
				ItemInfoCONT& item = items[row_n];
				ImGui::PushID(item.formId + 0x1000000);
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (item.invCount > 0) {
					if (item.isCrime) {
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
					} else {
					}
					auto openFlag = ImGui::TreeNode(item.formIdStr.c_str(), "%s", item.name.c_str());

					if (item.isCrime) {
						ImGui::PopStyleColor();
					} else {
					}

					if (openFlag) {
						for (int i2 = 0; i2 < item.invCount; i2++) {
							auto& inv = item.invs[i2];
							char buf[80];

							if (inv.count > 1) {
								snprintf(buf, 80, item.isEnchanted ? "%s" ICON_MDI_FLASH " (%d)" : "%s (%d)", inv.name.c_str(), inv.count);
							} else {
								snprintf(buf, 80, item.isEnchanted ? "%s" ICON_MDI_FLASH : "%s", inv.name.c_str());
							}

							if (inv.isCrime) {
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
								if (ImGui::Selectable(buf, false)) {
									//auto player = RE::PlayerCharacter::GetSingleton();
									//item.ptr->RemoveItem(inv.ptr, inv.count, RE::ITEM_REMOVE_REASON::kSteal, 0, player);
									RemoveItemACHR(nullptr, item.ptr, inv.ptr, inv.count, false);
								}
								ImGui::PopStyleColor();
							} else {
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
								if (ImGui::Selectable(buf, false)) {
									/*auto player = RE::PlayerCharacter::GetSingleton();
									item.ptr->RemoveItem(inv.ptr, inv.count, RE::ITEM_REMOVE_REASON::kRemove, 0, player);*/
									RemoveItemACHR(nullptr, item.ptr, inv.ptr, inv.count, false);
								}
								ImGui::PopStyleColor();
							}
						}
						ImGui::TreePop();
					}

				} else {
					myTextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s[空]", item.name.c_str());
				}

				ImGui::TableNextColumn();
				ImGui::Text("%d", item.invCount);

				if (show_items_window_direction) {
					ImGui::TableNextColumn();
					switch (item.direction) {
					case 1:  //前
						ImGui::Text(" \uf062%.0f", item.distance);
						break;
					case 2:  //左
						ImGui::Text(" \uf060%.0f", item.distance);
						break;
					case 3:
						ImGui::Text(" \uf063%.0f", item.distance);
						break;
					case 4:
						ImGui::Text(" \uf061%.0f", item.distance);
						break;
					default:
						ImGui::Text(" %.0f", item.distance);
						break;
					}
				}

				if (show_items_window_formid) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", item.baseFormIdStr.c_str());
				}

				ImGui::TableNextColumn();
				if (show_items_window_settings) {
					if (ImGui::SmallButton("\uf101")) {
						std::string commandStr = "player.moveto ";
						commandStr.append(item.formIdStr);
						ScriptUtil::ExecuteCommand(commandStr);
						if (activeItems) {
							activeItems = false;
						}
					}
				}

				if (trackPtrs.find(item.ptr) == trackPtrs.end()) {
					if (show_items_window_settings) {
						ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					}
					if (ImGui::SmallButton(ICON_MDI_MAP_MARKER_RADIUS)) {
						trackPtrs.insert(item.ptr);
					}
				}

				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}

	void __fastcall buildItemInfoBOOK(int count, ItemInfoBOOK* items, RE::FormType formType)
	{
		static ImGuiTableFlags flagsItem =
			ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;

		const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
		int columnCount = 4;

		if (show_items_window_refid) {
			columnCount++;
		}

		// 显示formid
		if (show_items_window_formid) {
			columnCount++;
		}
		columnCount++;

		if (show_items_window_direction) {
			columnCount++;
		}

		if (show_items_window_file) {
			columnCount++;
		}

		if (ImGui::BeginTable(("tableItem3" + std::to_string((int)formType)).c_str(), columnCount, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 15, TEXT_BASE_HEIGHT * show_inv_window_height), 0.0f)) {
			ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_1);
			ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_5);
			ImGui::TableSetupColumn("价值", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 35.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_2);
			ImGui::TableSetupColumn("重量", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_3);

			if (show_items_window_direction) {
				ImGui::TableSetupColumn("方位", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_8);
			}

			if (show_items_window_refid) {
				ImGui::TableSetupColumn("REFID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_9);
			}
			if (show_items_window_formid) {
				ImGui::TableSetupColumn("FORMID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_4);
			}
			if (show_items_window_file) {
				ImGui::TableSetupColumn("MOD", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_7);
			}
			//if (show_items_window_settings) {
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_6);
			//}
			ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
			ImGui::TableHeadersRow();

			ImGuiListClipper clipper;
			clipper.Begin(count);
			while (clipper.Step())
				for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
					ItemInfoBOOK& item = items[row_n];
					ImGui::PushID(item.formId + 0x1000000);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if (item.isCrime) {
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
						if (ImGui::Selectable(item.isRead ? (item.name + " " + ICON_MDI_EYE).c_str() : item.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
							if (item.ptr) {
								//if (!item.isDeleted) {
								if (!item.ptr->IsMarkedForDeletion()) {
									addItem(nullptr, item.ptr, 1);
									//item.isDeleted = true;
								}
								//}
							}
						}
						ImGui::PopStyleColor();
					} else {
						if (ImGui::Selectable(item.isRead ? (item.name + " " + ICON_MDI_EYE).c_str() : item.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
							if (item.ptr) {
								//if (!item.isDeleted) {
								if (!item.ptr->IsMarkedForDeletion()) {
									//auto player = RE::PlayerCharacter::GetSingleton();
									addItem(nullptr, item.ptr, 1);
									//item.isDeleted = true;
								}
								//}
							}
						}
					}

					ImGui::TableNextColumn();
					ImGui::Text("%s", item.formTypeStr.c_str());

					ImGui::TableNextColumn();
					ImGui::Text("%d", item.gold);

					ImGui::TableNextColumn();
					ImGui::Text("%.1f", item.weight);

					if (show_items_window_direction) {
						ImGui::TableNextColumn();
						switch (item.direction) {
						case 1:  //前
							ImGui::Text(" \uf062%.0f", item.distance);
							break;
						case 2:  //左
							ImGui::Text(" \uf060%.0f", item.distance);
							break;
						case 3:
							ImGui::Text(" \uf063%.0f", item.distance);
							break;
						case 4:
							ImGui::Text(" \uf061%.0f", item.distance);
							break;
						default:
							ImGui::Text(" %.0f", item.distance);
							break;
						}
					}

					if (show_items_window_refid) {
						ImGui::TableNextColumn();
						ImGui::Text("%s", item.formIdStr.c_str());
					}
					if (show_items_window_formid) {
						ImGui::TableNextColumn();
						ImGui::Text("%s", item.baseFormIdStr.c_str());
					}
					if (show_items_window_file) {
						ImGui::TableNextColumn();
						ImGui::Text("%s", item.filename.c_str());
					}

					ImGui::TableNextColumn();
					if (show_items_window_settings) {
						if (ImGui::SmallButton(ICON_MDI_EYE_REMOVE_OUTLINE)) {
							bool exist = false;
							for (const auto& excludeForm : excludeForms) {
								if (excludeForm.formId == item.baseFormId) {
									exist = true;
									break;
								}
							}
							if (!exist) {
								excludeForms.push_back({ item.baseFormId, item.name, item.formTypeStr });
							}
							excludeFormIds.insert(item.baseFormId);
						}

						ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
						if (ImGui::SmallButton("\uf101")) {
							/*	char buf[80];
							snprintf(buf, 80, "%0.2f, %0.2f, %0.2f", x * screenWidth, y * screenHeight, z);
							RE::DebugNotification(buf, NULL, false);*/

							//return;
							std::string commandStr = "player.moveto ";
							commandStr.append(item.formIdStr);

							ScriptUtil::ExecuteCommand(commandStr);
							if (activeItems) {
								activeItems = false;
							}
						}
					}

					if (trackPtrs.find(item.ptr) == trackPtrs.end()) {
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
						}
						if (ImGui::SmallButton(ICON_MDI_MAP_MARKER_RADIUS)) {
							trackPtrs.insert(item.ptr);
						}
					}

					ImGui::PopID();
				}
			ImGui::EndTable();
		}
	}

	void __fastcall buildItemInfo(int count, ItemInfo* items, RE::FormType formType)
	{
		static ImGuiTableFlags flagsItem =
			ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;

		const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
		int columnCount = 3;
		switch (formType) {
		case RE::FormType::Flora:
		case RE::FormType::Tree:
		case RE::FormType::Activator:
		case RE::FormType::KeyMaster:
			columnCount = 1;
			break;
		case RE::FormType::Ammo:
			columnCount = 2;
			break;
		case RE::FormType::None:
			columnCount = 4;
			break;
		default:
			break;
		}

		if (show_items_window_refid) {
			columnCount++;
		}

		// 显示formid
		if (show_items_window_formid) {
			columnCount++;
		}
		//if (show_items_window_settings) {
		columnCount++;
		//}
		if (show_items_window_direction) {
			columnCount++;
		}

		if (show_items_window_file) {
			columnCount++;
		}

		if (ImGui::BeginTable(("tableItem3" + std::to_string((int)formType)).c_str(), columnCount, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 15, TEXT_BASE_HEIGHT * show_inv_window_height), 0.0f)) {
			//ImGui::TableSetupColumn("已装备", ImGuiTableColumnFlags_WidthFixed, 40.0f, PlayerInfoColumnID_ID);
			ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_1);
			switch (formType) {
			case RE::FormType::Flora:
			case RE::FormType::Tree:
			case RE::FormType::Activator:
			case RE::FormType::KeyMaster:
				break;
			default:
				ImGui::TableSetupColumn("价值", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 35.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_2);
				break;
			}
			switch (formType) {
			case RE::FormType::Flora:
			case RE::FormType::Tree:
			case RE::FormType::Activator:
			case RE::FormType::Ammo:
			case RE::FormType::None:
			case RE::FormType::KeyMaster:
				break;
			default:
				ImGui::TableSetupColumn("重量", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_3);
				break;
			}
			switch (formType) {
			case RE::FormType::Container:
				ImGui::TableSetupColumn("数量", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_2);
				break;
			default:
				break;
			}

			if (show_items_window_direction) {
				ImGui::TableSetupColumn("方位", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_8);
			}

			switch (formType) {
			case RE::FormType::None:
				ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_5);
				break;
			default:
				break;
			}

			if (show_items_window_refid) {
				ImGui::TableSetupColumn("REFID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_9);
			}
			if (show_items_window_formid) {
				ImGui::TableSetupColumn("FORMID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_4);
			}
			if (show_items_window_file) {
				ImGui::TableSetupColumn("MOD", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_7);
			}
			//if (show_items_window_settings) {
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_6);
			//}
			ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
			ImGui::TableHeadersRow();

			ImGuiListClipper clipper;
			clipper.Begin(count);
			while (clipper.Step())
				for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
					ItemInfo& item = items[row_n];
					ImGui::PushID(item.formId + 0x1000000);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if (item.isHarvested && (formType == RE::FormType::Flora || formType == RE::FormType::Tree)) {
						myTextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s[已收获]", item.name.c_str());
					} else if (item.isCrime) {
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
						if (ImGui::Selectable(item.isEnchanted ? (item.name + ICON_MDI_FLASH).c_str() : item.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
							if (item.ptr) {
								if (formType == RE::FormType::Flora) {
									if (!(item.ptr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested)) {
										auto flora = item.ptr->GetBaseObject()->As<RE::TESFlora>();
										if (flora) {
											auto player = RE::PlayerCharacter::GetSingleton();
											flora->Activate(item.ptr, player, 0, flora->produceItem, 1);
										}
									}
								} else if (formType == RE::FormType::Tree) {
									if (!(item.ptr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested)) {
										auto tree = item.ptr->GetBaseObject()->As<RE::TESObjectTREE>();
										if (tree) {
											auto player = RE::PlayerCharacter::GetSingleton();
											tree->Activate(item.ptr, player, 0, tree->produceItem, 1);
										}
									}
								} else if (formType == RE::FormType::Activator) {
										char buf[120];
										snprintf(buf, 120, "%X.Activate player", item.formId);
										ScriptUtil::ExecuteCommand(buf);
								} else {
									if (!item.ptr->IsMarkedForDeletion()) {
										addItem(nullptr, item.ptr, 1);
									}
								}
							}
						}
						ImGui::PopStyleColor();
					} else {
						if (ImGui::Selectable(item.isEnchanted ? (item.name + ICON_MDI_FLASH).c_str() : item.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
							if (item.ptr) {
								if (formType == RE::FormType::Flora) {
									if (!(item.ptr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested)) {
										auto flora = item.ptr->GetBaseObject()->As<RE::TESFlora>();
										if (flora) {
											auto player = RE::PlayerCharacter::GetSingleton();
											flora->Activate(item.ptr, player, 0, flora->produceItem, 1);
										}
									}
								} else if (formType == RE::FormType::Tree) {
									if (!(item.ptr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested)) {
										auto tree = item.ptr->GetBaseObject()->As<RE::TESObjectTREE>();
										if (tree) {
											auto player = RE::PlayerCharacter::GetSingleton();
											tree->Activate(item.ptr, player, 0, tree->produceItem, 1);
										}
									}
								} else if (formType == RE::FormType::Activator) {
											char buf[120];
											snprintf(buf, 120, "%X.Activate player", item.formId);
											ScriptUtil::ExecuteCommand(buf);
								} else {
									if (!item.ptr->IsMarkedForDeletion()) {
										addItem(nullptr, item.ptr, 1);
									}
								}
							}

							/*			logger::debug(
								std::to_string(item.ptr->IsMarkedForDeletion()) 
								+ " - " + std::to_string(item.ptr->IsDeleted()) 
								+ " - " + std::to_string(item.ptr->IsDisabled()) 
								+ " - " + std::to_string(item.ptr->Is3rdPersonVisible()) 
								+ " - " + std::to_string(item.ptr->IsBoundObject()) 
								+ " - " + std::to_string(item.ptr->IsInitiallyDisabled()) 
							);*/
						}
					}
					switch (formType) {
					case RE::FormType::Flora:
					case RE::FormType::Tree:
					case RE::FormType::Activator:
					case RE::FormType::KeyMaster:
						break;
					default:
						ImGui::TableNextColumn();
						ImGui::Text("%d", item.gold);
						break;
					}
					switch (formType) {
					case RE::FormType::Flora:
					case RE::FormType::Tree:
					case RE::FormType::Activator:
					case RE::FormType::KeyMaster:
					case RE::FormType::Ammo:
					case RE::FormType::None:
						break;
					default:
						ImGui::TableNextColumn();
						ImGui::Text("%.1f", item.weight);
						break;
					}

					if (show_items_window_direction) {
						ImGui::TableNextColumn();
						switch (item.direction) {
						case 1:  //前
							ImGui::Text(" \uf062%.0f", item.distance);
							break;
						case 2:  //左
							ImGui::Text(" \uf060%.0f", item.distance);
							break;
						case 3:
							ImGui::Text(" \uf063%.0f", item.distance);
							break;
						case 4:
							ImGui::Text(" \uf061%.0f", item.distance);
							break;
						default:
							ImGui::Text(" %.0f", item.distance);
							break;
						}
					}

					switch (formType) {
					case RE::FormType::None:
						ImGui::TableNextColumn();
						ImGui::Text("%s", item.formTypeStr.c_str());
						break;
					default:
						break;
					}

					if (show_items_window_refid) {
						ImGui::TableNextColumn();
						ImGui::Text("%s", item.formIdStr.c_str());
					}
					if (show_items_window_formid) {
						ImGui::TableNextColumn();
						ImGui::Text("%s", item.baseFormIdStr.c_str());
					}
					if (show_items_window_file) {
						ImGui::TableNextColumn();
						ImGui::Text("%s", item.filename.c_str());
					}

					ImGui::TableNextColumn();
					if (show_items_window_settings) {
						if (ImGui::SmallButton(ICON_MDI_EYE_REMOVE_OUTLINE)) {
							bool exist = false;
							for (const auto& excludeForm : excludeForms) {
								if (excludeForm.formId == item.baseFormId) {
									exist = true;
									break;
								}
							}
							if (!exist) {
								excludeForms.push_back({ item.baseFormId, item.name, item.formTypeStr });
							}
							excludeFormIds.insert(item.baseFormId);
						}

						ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
						if (ImGui::SmallButton("\uf101")) {
							std::string commandStr = "player.moveto ";
							commandStr.append(item.formIdStr);
							ScriptUtil::ExecuteCommand(commandStr);
							if (activeItems) {
								activeItems = false;
							}
						}
					}

					if (trackPtrs.find(item.ptr) == trackPtrs.end()) {
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
						}
						if (ImGui::SmallButton(ICON_MDI_MAP_MARKER_RADIUS)) {
							trackPtrs.insert(item.ptr);
						}
					}

					ImGui::PopID();
				}
			ImGui::EndTable();
		}
	}

	void __fastcall takeAllItem(int count, ItemInfo* items, RE::FormType formType)
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		if (formType == RE::FormType::Flora) {
			for (int i = 0; i < count; i++) {
				ItemInfo& item = items[i];
				if (!item.isCrime) {
					if (!(item.ptr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested)) {
						auto flora = item.ptr->GetBaseObject()->As<RE::TESFlora>();
						if (flora) {
							flora->Activate(item.ptr, player, 0, flora->produceItem, 1);
						}
					}
				}
			}
		} else if (formType == RE::FormType::Tree) {
			for (int i = 0; i < count; i++) {
				ItemInfo& item = items[i];
				if (!item.isCrime) {
					if (!(item.ptr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested)) {
						auto tree = item.ptr->GetBaseObject()->As<RE::TESObjectTREE>();
						if (tree) {
							tree->Activate(item.ptr, player, 0, tree->produceItem, 1);
						}
					}
				}
			}
		} else {
			for (int i = 0; i < count; i++) {
				ItemInfo& item = items[i];
				if (!item.isCrime) {
					if (item.ptr) {
						//if (!item.isDeleted) {
						if (!item.ptr->IsMarkedForDeletion()) {
							addItem(nullptr, item.ptr, 1);
							//item.isDeleted = true;
						}
						//}
					}
				}
			}
		}
	}

	void __fastcall takeAllItemBOOK(int count, ItemInfoBOOK* items, RE::FormType formType)
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		for (int i = 0; i < count; i++) {
			ItemInfoBOOK& item = items[i];
			if (!item.isCrime) {
				if (item.ptr) {
					//if (!item.isDeleted) {
					if (!item.ptr->IsMarkedForDeletion()) {
						addItem(nullptr, item.ptr, 1);
						//item.isDeleted = true;
					}
					//}
				}
			}
		}
	}

	void __fastcall render()
	{
		//第一次加载游戏后
		if (!startflag && !active) {
			return;
		}
		// 当打开菜单时不显示
		if (isOpenCursorMenu && !active) {
			return;
		}
		// 当打开菜单时不显示
		if (isMainMenu && !active) {
			return;
		}
		// 当打开菜单时不显示
		if (isLoadWaitSpinner && !active) {
			return;
		}
		// 当打开菜单时不显示
		if (isFaderMenu && !active) {
			return;
		}

		if (isGameLoading && !active) {
			return;
		}

		ImGuiWindowFlags window_flags = 0;
		if (no_titlebar)
			window_flags |= ImGuiWindowFlags_NoTitleBar;
		if (no_resize)
			window_flags |= ImGuiWindowFlags_NoResize;
		if (no_collapse)
			window_flags |= ImGuiWindowFlags_NoCollapse;
		if (no_background)
			window_flags |= ImGuiWindowFlags_NoBackground;
		if (auto_resize)
			window_flags |= ImGuiWindowFlags_AlwaysAutoResize;

		auto style = ImGui::GetStyle();
		style.WindowBorderSize = window_border ? 1.0f : 0.0f;
		style.FrameBorderSize = frame_border ? 1.0f : 0.0f;

		// 追踪
		if (trackPtrs.size() > 0 || trackActorPtrs.size() > 0) {
			if (!(isOpenCursorMenu || isMainMenu || isLoadWaitSpinner || isFaderMenu)) {
				RE::NiPointer<RE::NiCamera> camera = getCamera();
				RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
				if (camera) {
					RE::NiCamera* ca = camera.get();
					if (ca) {
						for (const auto& ptr : trackPtrs) {
							float x1;
							float y1;
							float z1;
							if (ptr->IsDeleted() || player->currentLocation != ptr->GetCurrentLocation()) {
								trackPtrs.erase(ptr);
								break;
							}
							auto p = ptr->GetPosition();
							bool result = ca->WorldPtToScreenPt3(ca->worldToCam, ca->port, p, x1, y1, z1, 1e-5f);
							if (!result) {
								int i = 1;
							}
							if (x1 > 0 && y1 > 0 && z1 > 0) {
								trackX1 = x1 * screenWidth;
								trackY1 = screenHeight - y1 * screenHeight;
								if (trackX1 > 0 && trackX1 < screenWidth && trackY1 > 0 && trackY1 < screenHeight) {
									ImGui::SetNextWindowPos(ImVec2(trackX1 - 40, trackY1));

									char buf[32];
									snprintf(buf, 32, "%p", ptr);
									ImGui::Begin(buf, nullptr,
										ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);

									ImGui::Text(ICON_MDI_MAP_MARKER_RADIUS " %0.1fm", ValueUtil::calculateDistance(ptr->GetPosition(), player->GetPosition()) / 100.0f);
									ImGui::End();
								}
							}
						}

						for (const auto& ptr : trackActorPtrs) {
							float x1;
							float y1;
							float z1;
							if (ptr->IsDeleted() || player->currentLocation != ptr->GetCurrentLocation()) {
								trackActorPtrs.erase(ptr);
								break;
							}
							auto p = ptr->GetPosition();
							bool result = ca->WorldPtToScreenPt3(ca->worldToCam, ca->port, p, x1, y1, z1, 1e-5f);
							if (!result) {
								int i = 1;
							}
							if (x1 > 0 && y1 > 0 && z1 > 0) {
								trackX1 = x1 * screenWidth;
								trackY1 = screenHeight - y1 * screenHeight;
								if (trackX1 > 0 && trackX1 < screenWidth && trackY1 > 0 && trackY1 < screenHeight) {
									ImGui::SetNextWindowPos(ImVec2(trackX1 - 40, trackY1));

									char buf[32];
									snprintf(buf, 32, "%p", ptr);
									ImGui::Begin(buf, nullptr,
										ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

									ImGui::Text(ICON_MDI_MAP_MARKER_RADIUS " %0.1fm", ValueUtil::calculateDistance(ptr->GetPosition(), player->GetPosition()) / 100.0f);
									ImGui::End();
								}
							}
						}
					}
				}
			}
		}

		if (show_player_base_info_window) {
			if (show_player_base_info_window_sep) {
				progress = playerInfo.kHealth / (playerInfo.kHealthBase == 0 ? 1 : playerInfo.kHealthBase);
				progress2 = playerInfo.kStamina / (playerInfo.kStaminaBase == 0 ? 1 : playerInfo.kStaminaBase);
				progress3 = playerInfo.kMagicka / (playerInfo.kMagickaBase == 0 ? 1 : playerInfo.kMagickaBase);

				ImGui::Begin("生命", nullptr, window_flags);

				ImGui::Text(ICON_MDI_HEART_HALF_FULL " 生命:");
				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
				if (progress > 0.15) {
					ImGui::Text("%.1f / %.0f ", playerInfo.kHealth, playerInfo.kHealthBase);
				} else {
					myTextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.1f / %.0f ", playerInfo.kHealth, playerInfo.kHealthBase);
				}

				ImGui::End();
				ImGui::Begin("耐力", nullptr, window_flags);

				ImGui::Text(ICON_MDI_RUN_FAST " 耐力:");
				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
				if (progress2 > 0.15) {
					ImGui::Text("%.1f / %.0f ", playerInfo.kStamina, playerInfo.kStaminaBase);
				} else {
					myTextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.1f / %.0f ", playerInfo.kStamina, playerInfo.kStaminaBase);
				}
				ImGui::End();
				ImGui::Begin("魔法", nullptr, window_flags);

				ImGui::Text(ICON_MDI_STAR_FOUR_POINTS_OUTLINE " 魔法:");
				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
				if (progress3 > 0.15) {
					ImGui::Text("%.1f / %.0f ", playerInfo.kMagicka, playerInfo.kMagickaBase);
				} else {
					myTextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.1f / %.0f ", playerInfo.kMagicka, playerInfo.kMagickaBase);
				}
				ImGui::End();

			} else {
				ImGui::Begin("人物状态", nullptr, window_flags);

				progress = playerInfo.kHealth / (playerInfo.kHealthBase == 0 ? 1 : playerInfo.kHealthBase);
				progress2 = playerInfo.kStamina / (playerInfo.kStaminaBase == 0 ? 1 : playerInfo.kStaminaBase);
				progress3 = playerInfo.kMagicka / (playerInfo.kMagickaBase == 0 ? 1 : playerInfo.kMagickaBase);

				if (flag_process) {
					ImGui::Text(ICON_MDI_HEART_HALF_FULL " 生命:");
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					char buf[32];
					sprintf(buf, "%d/%d", (int)playerInfo.kHealth, (int)playerInfo.kHealthBase);
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorProgress);
					ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), buf);

					ImGui::Text(ICON_MDI_RUN_FAST " 耐力:");
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					char buf2[32];
					sprintf(buf2, "%d/%d", (int)playerInfo.kStamina, (int)playerInfo.kStaminaBase);
					ImGui::ProgressBar(progress2, ImVec2(0.f, 0.f), buf2);

					ImGui::Text(ICON_MDI_STAR_FOUR_POINTS_OUTLINE " 魔法:");
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					char buf3[32];
					sprintf(buf3, "%d/%d", (int)playerInfo.kMagicka, (int)playerInfo.kMagickaBase);
					ImGui::ProgressBar(progress3, ImVec2(0.f, 0.f), buf3);
					ImGui::PopStyleColor();

				} else {
					ImGui::Text(ICON_MDI_HEART_HALF_FULL " 生命:");
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					if (progress > 0.15) {
						ImGui::Text("%.1f / %.0f ", playerInfo.kHealth, playerInfo.kHealthBase);
					} else {
						myTextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.1f / %.0f ", playerInfo.kHealth, playerInfo.kHealthBase);
					}
					ImGui::Text(ICON_MDI_RUN_FAST " 耐力:");
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					if (progress2 > 0.15) {
						ImGui::Text("%.1f / %.0f ", playerInfo.kStamina, playerInfo.kStaminaBase);
					} else {
						myTextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.1f / %.0f ", playerInfo.kStamina, playerInfo.kStaminaBase);
					}
					ImGui::Text(ICON_MDI_STAR_FOUR_POINTS_OUTLINE " 魔法:");
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					if (progress3 > 0.15) {
						ImGui::Text("%.1f / %.0f ", playerInfo.kMagicka, playerInfo.kMagickaBase);
					} else {
						myTextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.1f / %.0f ", playerInfo.kMagicka, playerInfo.kMagickaBase);
					}
				}
				ImGui::End();
			}
		}

		if (show_player_info_window) {
			ImGui::Begin("人物属性", nullptr, window_flags);

			ImGui::Text(ICON_MDI_SWORD " 伤害: %s", playerInfo.DamageStr.c_str());
			ImGui::Text(ICON_MDI_SHIELD_HALF_FULL " 防御: %.2f", playerInfo.kDamageResist);
			ImGui::Text(ICON_MDI_WATER " 毒抗: %.1f%%", playerInfo.kPoisonResist);
			ImGui::Text(ICON_MDI_FIRE " 火抗: %.1f%%", playerInfo.kResistFire);
			ImGui::Text(ICON_MDI_LIGHTNING_BOLT_OUTLINE " 雷抗: %.1f%%", playerInfo.kResistShock);
			ImGui::Text(ICON_MDI_SNOWFLAKE " 冰抗: %.1f%%", playerInfo.kResistFrost);
			ImGui::Text(ICON_MDI_MAGIC_STAFF " 法抗: %.1f%%", playerInfo.kResistMagic);
			ImGui::Text(ICON_MDI_PILL " 抵抗力: %.1f%%", playerInfo.kResistDisease);
			//ImGui::Text("criticalChance:%d", (int)kCriticalChance);
			ImGui::End();
		}

		if (show_player_mod_window) {
			ImGui::Begin("属性加成", nullptr, window_flags);

			ImGui::Text(ICON_MDI_AXE " 单手: %.1f%%", playerInfo.kOneHandedModifier);
			ImGui::Text(ICON_MDI_AXE_BATTLE " 双手: %.1f%%", playerInfo.kTwoHandedModifier);
			ImGui::Text(ICON_MDI_BOW_ARROW " 弓箭: %.1f%%", playerInfo.kMarksmanModifier);
			ImGui::Text(ICON_MDI_ANVIL " 锻造: %.1f%%", playerInfo.kSmithingModifier);
			ImGui::Text(ICON_MDI_ANVIL " 锻造(power): %.1f%%", playerInfo.kSmithingPowerModifier);
			ImGui::Text(ICON_MDI_ANVIL " 锻造(skill): %.1f%%", playerInfo.kSmithingSkillAdvance);
			ImGui::Text(ICON_MDI_BOWL_MIX " 炼金: %.1f%%", playerInfo.kAlchemyModifier);
			ImGui::Text(ICON_MDI_BOWL_MIX " 炼金(power): %.1f%%", playerInfo.kAlchemyPowerModifier);
			ImGui::Text(ICON_MDI_BOWL_MIX " 炼金(skill): %.1f%%", playerInfo.kAlchemySkillAdvance);
			ImGui::Text(ICON_MDI_FLASH " 附魔: %.1f%%", playerInfo.kEnchantingModifier);
			ImGui::Text(ICON_MDI_FLASH " 附魔(power): %.1f%%", playerInfo.kEnchantingPowerModifier);
			ImGui::Text(ICON_MDI_FLASH " 附魔(skill): %.1f%%", playerInfo.kEnchantingSkillAdvance);
			ImGui::End();
		}

		if (show_player_weapon_window) {
			ImGui::Begin("武器", nullptr, window_flags);

			if (leftWeaponInfo.isExist) {
				if (active) {
					if (ImGui::TreeNode(leftWeaponInfo.treeId.c_str())) {
						ImGui::Text("类型: %s", leftWeaponInfo.formTypeName.c_str());
						ImGui::Text("ID: %s", leftWeaponInfo.formID.c_str());
						ImGui::SameLine(0, 0);
						if (ImGui::SmallButton("卸载")) {
							std::string commandStr = "player.unequipItem ";
							commandStr.append(leftWeaponInfo.formID);
							ScriptUtil::ExecuteCommand(commandStr);
						}

						switch (leftWeaponInfo.formType) {
						case RE::FormType::Weapon:
							{
								ImGui::Text("武器类型: %s", leftWeaponInfo.weaponTypeName.c_str());
								ImGui::Text("武器伤害: %d", leftWeaponInfo.damage);
								ImGui::Text("暴击伤害: %d", leftWeaponInfo.critDamage);
								ImGui::Text("价格: %d", leftWeaponInfo.goldValue);
								ImGui::Text("属性值: %d$", leftWeaponInfo.value);
								ImGui::Text("重量: %.1f", leftWeaponInfo.weight);
								break;
							}
						case RE::FormType::Spell:
							{
								ImGui::Text("法术类型: %s", leftWeaponInfo.castingTypeName.c_str());
								ImGui::Text("法术类型: %s", leftWeaponInfo.spellTypeName.c_str());
								ImGui::Text("花费: %.1f", leftWeaponInfo.cost);
								ImGui::Text("施法时间: %.1f", leftWeaponInfo.time);
								break;
							}

						default:
							break;
						}

						//ImGui::Text("插槽: %s", item.equipSlotName.c_str());
						ImGui::TreePop();
					}
				} else {
					ImGui::Text(ICON_MDI_SWORD " %s", leftWeaponInfo.treeId.c_str());
				}
			}

			if (rightWeaponInfo.isExist) {
				// 不显示双手武器
				if (!rightWeaponInfo.isTwoHand) {
					if (active) {
						if (ImGui::TreeNode(rightWeaponInfo.treeId.c_str())) {
							ImGui::Text("类型: %s", rightWeaponInfo.formTypeName.c_str());
							ImGui::Text("ID: %s", rightWeaponInfo.formID.c_str());
							ImGui::SameLine(0, 0);
							if (ImGui::SmallButton("卸载")) {
								std::string commandStr = "player.unequipItem ";
								commandStr.append(rightWeaponInfo.formID);
								ScriptUtil::ExecuteCommand(commandStr);
							}

							switch (rightWeaponInfo.formType) {
							case RE::FormType::Weapon:
								{
									ImGui::Text("武器类型: %s", rightWeaponInfo.weaponTypeName.c_str());
									ImGui::Text("武器伤害: %d", rightWeaponInfo.damage);
									ImGui::Text("暴击伤害: %d", rightWeaponInfo.critDamage);
									ImGui::Text("价格: %d", rightWeaponInfo.goldValue);
									ImGui::Text("属性值: %d$", rightWeaponInfo.value);
									ImGui::Text("重量: %.1f", rightWeaponInfo.weight);
									break;
								}
							case RE::FormType::Spell:
								{
									ImGui::Text("法术类型: %s", rightWeaponInfo.castingTypeName.c_str());
									ImGui::Text("法术类型: %s", rightWeaponInfo.spellTypeName.c_str());
									ImGui::Text("花费: %.1f", rightWeaponInfo.cost);
									ImGui::Text("施法时间: %.1f", rightWeaponInfo.time);
									break;
								}
							default:
								break;
							}

							ImGui::TreePop();
						}
					} else {
						ImGui::Text(ICON_MDI_SWORD " %s", rightWeaponInfo.treeId.c_str());
					}
				}
			}

			if (ammoInfo.isExist) {
				if (active) {
					if (ImGui::TreeNode(ammoInfo.treeId.c_str())) {
						ImGui::Text("类型: %s", ammoInfo.formTypeName.c_str());
						ImGui::Text("ID: %s", ammoInfo.formID.c_str());
						ImGui::SameLine(0, 0);
						if (ImGui::SmallButton("卸载")) {
							std::string commandStr = "player.unequipItem ";
							commandStr.append(ammoInfo.formID);
							ScriptUtil::ExecuteCommand(commandStr);
						}
						ImGui::Text("伤害: %.1f", ammoInfo.damage);
						ImGui::Text("价格: %d", ammoInfo.goldValue);
						ImGui::Text("属性值: %d$", ammoInfo.value);
						ImGui::Text("重量: %.1f", ammoInfo.weight);

						ImGui::TreePop();
					}
				} else {
					ImGui::Text(ICON_MDI_ARROW_PROJECTILE " %s", ammoInfo.treeId.c_str());
				}
			}

			ImGui::End();
		}

		if (show_player_armor_window) {
			ImGui::Begin("装备", nullptr, window_flags);

			int unWornArmosAlertSlots = 0;

			for (int i = 0; i <= 31; i++) {
				auto& item = wornArmos[i];

				if (item.isExist) {
					if (item.isSpeacilSlotAlert) {
						// 变黄色
						//imgui.igPushStyleColor
					}

					if (active) {
						if (ImGui::TreeNode(item.treeId.c_str())) {
							ImGui::Text("类型: %s", item.formTypeName.c_str());
							ImGui::Text("装备类型: %s", item.armorTypeName.c_str());
							ImGui::Text("ID: %s", item.formID.c_str());
							ImGui::SameLine(0, 0);
							if (ImGui::SmallButton("卸载")) {
								std::string commandStr = "player.unequipItem ";
								commandStr.append(item.formID);
								ScriptUtil::ExecuteCommand(commandStr);
							}
							ImGui::Text("价格: %d$", item.goldValue);
							ImGui::Text("属性值: %d", item.value);
							ImGui::Text("装备等级: %.2f", item.armorRating);
							ImGui::Text("插槽: %s", item.equipSlotName.c_str());
							ImGui::Text("重量: %.1f", item.weight);
							ImGui::TreePop();
						}
					} else {
						ImGui::Text("%s", item.treeId.c_str());
					}

				} else {
					if (item.isMainSlotAlert) {
						unWornArmosAlertSlots += 1 << i;
						// 标记未装备
					}
				}
			}

			// 未装备插槽显示红色未装备
			for (int i = 0; i <= 31; i++) {
				int mask = 1 << i;
				if ((unWornArmosAlertSlots & mask) == mask) {
					myTextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "未装备 | %s", wornArmos[i].equipSlotName.c_str());
				}
			}

			ImGui::End();
		}

		if (show_player_debug_window) {
			ImGui::Begin("其他信息", nullptr, window_flags);
			ImGui::Text(ICON_MDI_MAP_MARKER_RADIUS " 位置: %s", playerInfo.location.c_str());
			// z高度
			ImGui::Text(ICON_MDI_AXIS_ARROW " 坐标: [%.1f,%.1f,%.1f]", playerInfo.Position.x / 100.0f, playerInfo.Position.y / 100.0f, playerInfo.Position.z / 100.0f);
			//ImGui::SameLine(0, 0);
			// z 是0~2Π
			ImGui::Text(ICON_MDI_ANGLE_ACUTE " 视角: [%.2f,%.2f]", playerInfo.Angle.x, playerInfo.Angle.z);
			ImGui::End();
		}

		// 弃用
		if (show_enemy_window && false) {
			time_t now1 = time(NULL);
			ImGui::Begin("敌人信息", nullptr, window_flags);
			EnemyInfo* enemyInfo = getEnemyData();
			for (int i = 0; i < 50; i++) {
				EnemyInfo& item = enemyInfo[i];
				if (item.kHealth > 0) {
					if (now1 - item.updateTime < 40) {
						float enemyHealthRate = item.kHealth / (item.kHealthBase == 0 ? 1 : item.kHealthBase);

						ImGui::Text("[%d] %s [", item.level, item.name.c_str());
						ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
						if (enemyHealthRate > 0.85f) {
							myTextColored(ImVec4(0.0f, 1, 0.0f, 1.0f), "%.0f/%.0f ", item.kHealth, item.kHealthBase);
						} else if (enemyHealthRate < 0.20f) {
							myTextColored(ImVec4(1, 0, 0.0f, 1.0f), "%.0f/%.0f ", item.kHealth, item.kHealthBase);
						} else {
							ImGui::Text("%.0f/%.0f ", item.kHealth, item.kHealthBase);
						}
						ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
						ImGui::Text("%.0f ", item.kStamina);
						ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
						myTextColored(ImVec4(0.0f, 0, 1, 1.0f), "%.0f", item.kMagicka);
						ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
						ImGui::Text("] %d", item.isSentient);
						ImGui::Text("]");
					}
				}
			}
			ImGui::End();
		}

		if (show_npc_window) {
			ImGui::Begin("npc信息", nullptr, window_flags);
			//if (!isRefreshActorInfo) {
			if (getNpcCount() > 0) {
				if (active) {
					ImGui::Checkbox(ICON_MDI_ACCOUNT " npc", &show_npc);
				} else {
					if (show_npc) {
						ImGui::Text(ICON_MDI_ACCOUNT " npc");
					}
				}
				if (show_npc) {
					auto actorInfo = getNpcData();
					buildNpcInfo(active, actorInfo, getNpcCount());
				}
			}

			if (getHorseCount() > 0) {
				if (getNpcCount() > 0) {
					if (!no_background) {
						ImGui::Separator();
					} else {
						ImGui::Text(" ");
					}
				}
				if (active) {
					ImGui::Checkbox(ICON_MDI_HORSE_VARIANT " horse", &show_horse);
				} else {
					if (show_horse) {
						ImGui::Text(ICON_MDI_HORSE_VARIANT " horse");
					}
				}
				if (show_horse) {
					auto actorInfo = getHorseData();
					buildNpcInfo(active, actorInfo, getHorseCount());
				}
			}

			if (getEnemyCount() > 0) {
				if (getNpcCount() || getHorseCount()) {
					if (!no_background) {
						ImGui::Separator();
					} else {
						ImGui::Text(" ");
					}
				}
				if (active) {
					ImGui::Checkbox(ICON_MDI_SWORD_CROSS " enemy", &show_enemy);
				} else {
					if (show_enemy) {
						ImGui::Text(ICON_MDI_SWORD_CROSS " enemy");
					}
				}
				if (show_enemy) {
					auto actorInfo = getEnemy2Data();
					buildNpcInfo(active, actorInfo, getEnemyCount());
				}
			}

			if (getTeammateCount() > 0) {
				if (getNpcCount() || getEnemyCount() || getHorseCount()) {
					if (!no_background) {
						ImGui::Separator();
					} else {
						ImGui::Text(" ");
					}
				}

				if (active) {
					ImGui::Checkbox(ICON_MDI_SHIELD_ACCOUNT " team", &show_teammate);
				} else {
					if (show_teammate) {
						ImGui::Text(ICON_MDI_SHIELD_ACCOUNT " team");
					}
				}
				if (show_teammate) {
					auto actorInfo = getTeammateData();
					buildNpcInfo(active, actorInfo, getTeammateCount());
				}
			}
			//}
			ImGui::End();
		}

		if (show_inv_window) {
			if (active || !show_inv_window_active) {
				ImGui::Begin("防具信息", nullptr, window_flags);
				ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
				if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
					if (ImGui::BeginTabItem(ICON_MDI_SWORD " 武器", 0, 0)) {
						buildPlayerInvInfo(getPlayerInvWEAPCount(), getPlayerInvWEAPData());
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(ICON_MDI_SHIELD " 装备", 0, 0)) {
						buildPlayerInvInfo(getPlayerInvARMOCount(), getPlayerInvARMOData());
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(ICON_MDI_ARROW_PROJECTILE " 弹药", 0, 0)) {
						buildPlayerInvInfo(getPlayerInvAMMOCount(), getPlayerInvAMMOData());
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(ICON_MDI_SOURCE_BRANCH " 材料", 0, 0)) {
						buildPlayerInvInfo(getPlayerInvINGRCount(), getPlayerInvINGRData());
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem(ICON_MDI_BOTTLE_TONIC_PLUS_OUTLINE " 药水", 0, 0)) {
						buildPlayerInvInfo(getPlayerInvALCHCount(), getPlayerInvALCHData());
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(ICON_MDI_FOOD_DRUMSTICK " 食物", 0, 0)) {
						buildPlayerInvInfo(getPlayerInvFOODCount(), getPlayerInvFOODData());
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(ICON_MDI_BOOK_OPEN_VARIANT " 书信", 0, 0)) {
						buildPlayerInvInfo(getPlayerInvBOOKCount(), getPlayerInvBOOKData());
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(ICON_MDI_AXIS_ARROW " 杂项", 0, 0)) {
						buildPlayerInvInfo(getPlayerInvCount(), getPlayerInvData());
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}

				ImGui::Text("\uf0d6 %d", getPlayerGoldCount());
				//ImGui::Text("负重：%0.1f/%0.0f", playerInfo.equippedWeight, playerInfo.carryWeight);

				ImGui::End();
			}
		}

		if (show_items_window) {
			/*ImGui::Begin("物品信息", nullptr, window_flags);

			ImGui::End();*/
		}

		if (activeItems) {
			ImGuiWindowFlags window_flags2 = 0;
			if (no_titlebar)
				window_flags2 |= ImGuiWindowFlags_NoTitleBar;
			if (no_resize)
				window_flags2 |= ImGuiWindowFlags_NoResize;
			if (no_collapse)
				window_flags2 |= ImGuiWindowFlags_NoCollapse;
			if (no_background_items)
				window_flags2 |= ImGuiWindowFlags_NoBackground;
			if (auto_resize)
				window_flags2 |= ImGuiWindowFlags_AlwaysAutoResize;

			ImGui::Begin("附近物品信息", nullptr, window_flags2);
			ImGui::Text(ICON_MDI_MAP_MARKER_RADIUS " %s", playerInfo.location.c_str());
			if (show_items_window_formid) {
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
				ImGui::Text("[%08X]", playerInfo.locationId);
				if (playerInfo.parentLocationId != -1) {
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					ImGui::Text(" - %s", playerInfo.parentLocation.c_str());
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					ImGui::Text("[%08X]", playerInfo.parentLocationId);
				}
			}

			if (lotd::isLoad) {
				ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x * 3);
				if (lotd::isShow) {
					if (ImGui::SmallButton("艺术馆模式")) {
						lotd::isShow = !lotd::isShow;
					}

				} else {
					if (ImGui::SmallButton("普通模式")) {
						lotd::isShow = !lotd::isShow;
					}
				}
			}

			if (ImGui::BeginTable("tableItem", 5)) {
				if (lotd::isShow) {
					lotd::render();

				} else {
					if (getItemCountWEAP() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_SWORD " 武器(%d)", getItemCountWEAP());

						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##41")) {
								takeAllItem(getItemCountWEAP(), getItemsWEAP(), RE::FormType::Weapon);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##41", &show_items_window_auto_weap);
						} else {
							if (show_items_window_auto_weap) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountWEAP(), getItemsWEAP(), RE::FormType::Weapon);
					}
					if (getItemCountARMO() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_SHIELD " 装备(%d)", getItemCountARMO());

						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##47")) {
								takeAllItem(getItemCountARMO(), getItemsARMO(), RE::FormType::Armor);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##26", &show_items_window_auto_armo);
						} else {
							if (show_items_window_auto_armo) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountARMO(), getItemsARMO(), RE::FormType::Armor);
					}
					if (getItemCountAMMO() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_ARROW_PROJECTILE " 弹药(%d)", getItemCountAMMO());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##42")) {
								takeAllItem(getItemCountAMMO(), getItemsAMMO(), RE::FormType::Ammo);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##47", &show_items_window_auto_ammo);
						} else {
							if (show_items_window_auto_ammo) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountAMMO(), getItemsAMMO(), RE::FormType::Ammo);
					}
					if (getItemCountBOOK() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_BOOK_OPEN_VARIANT " 书信(%d)", getItemCountBOOK());

						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##43")) {
								takeAllItemBOOK(getItemCountBOOK(), getItemsBOOK(), RE::FormType::Book);
							}
						}
						buildItemInfoBOOK(getItemCountBOOK(), getItemsBOOK(), RE::FormType::Book);
					}
					if (getItemCountALCH() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_BOTTLE_TONIC_PLUS_OUTLINE " 药水(%d)", getItemCountALCH());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##44")) {
								takeAllItem(getItemCountALCH(), getItemsALCH(), RE::FormType::AlchemyItem);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##44", &show_items_window_auto_alch);
						} else {
							if (show_items_window_auto_alch) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountALCH(), getItemsALCH(), RE::FormType::AlchemyItem);
					}
					if (getItemCountFOOD() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_FOOD_DRUMSTICK " 食物(%d)", getItemCountFOOD());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##45")) {
								takeAllItem(getItemCountFOOD(), getItemsFOOD(), RE::FormType::PluginInfo);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##45", &show_items_window_auto_food);
						} else {
							if (show_items_window_auto_food) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountFOOD(), getItemsFOOD(), RE::FormType::PluginInfo);  // 临时用PluginInfo
					}
					if (getItemCountINGR() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_SOURCE_BRANCH " 材料(%d)", getItemCountINGR());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##48")) {
								takeAllItem(getItemCountINGR(), getItemsINGR(), RE::FormType::Ingredient);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##48", &show_items_window_auto_ingr);
						} else {
							if (show_items_window_auto_ingr) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountINGR(), getItemsINGR(), RE::FormType::Ingredient);
					}
					if (getItemCountSGEM() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_CARDS_DIAMOND " 灵魂石(%d)", getItemCountSGEM());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##53")) {
								takeAllItem(getItemCountSGEM(), getItemsSGEM(), RE::FormType::SoulGem);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##53", &show_items_window_auto_sgem);
						} else {
							if (show_items_window_auto_sgem) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountSGEM(), getItemsSGEM(), RE::FormType::SoulGem);
					}
					if (getItemCountKEYM() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_KEY " 钥匙(%d)", getItemCountKEYM());

						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##52")) {
								takeAllItem(getItemCountKEYM(), getItemsKEYM(), RE::FormType::KeyMaster);
							}
						}
						buildItemInfo(getItemCountKEYM(), getItemsKEYM(), RE::FormType::KeyMaster);
					}
					if (getItemCountSTON() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_DIAMOND_STONE " 宝石(%d)", getItemCountSTON());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##32-1")) {
								takeAllItem(getItemCountSTON(), getItemsSTON(), RE::FormType::Misc);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##32-1", &show_items_window_auto_ston);
						} else {
							if (show_items_window_auto_ston) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountSTON(), getItemsSTON(), RE::FormType::Misc);
					}
					if (getItemCountANVI() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_ANVIL " 矿锭(%d)", getItemCountANVI());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##32-2")) {
								takeAllItem(getItemCountANVI(), getItemsANVI(), RE::FormType::Misc);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##32-2", &show_items_window_auto_anvi);
						} else {
							if (show_items_window_auto_anvi) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountANVI(), getItemsANVI(), RE::FormType::Misc);
					}
					if (getItemCountANHD() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_BOX_CUTTER " 兽皮(%d)", getItemCountANHD());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##32-3")) {
								takeAllItem(getItemCountANHD(), getItemsANHD(), RE::FormType::Misc);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##32-3", &show_items_window_auto_anhd);
						} else {
							if (show_items_window_auto_anhd) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountANHD(), getItemsANHD(), RE::FormType::Misc);
					}
					if (getItemCountANPA() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_RABBIT " 战利品(%d)", getItemCountANPA());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##32-4")) {
								takeAllItem(getItemCountANPA(), getItemsANPA(), RE::FormType::Misc);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##32-4", &show_items_window_auto_anpa);
						} else {
							if (show_items_window_auto_anpa) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountANPA(), getItemsANPA(), RE::FormType::Misc);
					}
					if (getItemCountTOOL() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_TOOLS " 工具(%d)", getItemCountTOOL());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##32-5")) {
								takeAllItem(getItemCountTOOL(), getItemsTOOL(), RE::FormType::Misc);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##32-5", &show_items_window_auto_tool);
						} else {
							if (show_items_window_auto_tool) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountTOOL(), getItemsTOOL(), RE::FormType::Misc);
					}
					if (getItemCountMISC() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_PACKAGE_VARIANT_CLOSED " 杂项(%d)", getItemCountMISC());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##49")) {
								takeAllItem(getItemCountMISC(), getItemsMISC(), RE::FormType::Misc);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##49", &show_items_window_auto_misc);
						} else {
							if (show_items_window_auto_misc) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountMISC(), getItemsMISC(), RE::FormType::Misc);
					}
					if (getItemCountFLOR() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_BASKET_OUTLINE " 可收获(%d)", getItemCountFLOR());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##50")) {
								takeAllItem(getItemCountFLOR(), getItemsFLOR(), RE::FormType::Flora);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动收获##50", &show_items_window_auto_flor);
						} else {
							if (show_items_window_auto_flor) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountFLOR(), getItemsFLOR(), RE::FormType::Flora);
					}
					if (getItemCountTREE() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_FLOWER_TULIP_OUTLINE " 植物(%d)", getItemCountTREE());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							if (ImGui::SmallButton(ICON_MDI_ARCHIVE_ARROW_DOWN "##53")) {
								takeAllItem(getItemCountTREE(), getItemsTREE(), RE::FormType::Tree);
							}
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动收获##53", &show_items_window_auto_tree);
						} else {
							if (show_items_window_auto_tree) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemInfo(getItemCountTREE(), getItemsTREE(), RE::FormType::Tree);
					}
					if (getItemCountACHR() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_HUMAN_MALE " 尸体(%d)", getItemCountACHR());

						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##62", &show_items_window_auto_achr);
						} else {
							if (show_items_window_auto_achr) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemACHRInfo(getItemCountACHR(), getItemsACHR(), RE::FormType::ActorCharacter);
					}
					if (getItemCountCONT() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_ARCHIVE_OUTLINE " 箱子(%d)", getItemCountCONT());
						if (show_items_window_settings) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("自动拾取##28", &show_items_window_auto_cont);
						} else {
							if (show_items_window_auto_cont) {
								ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								ImGui::Text(ICON_MDI_AUTORENEW);
							}
						}
						buildItemCONTInfo(getItemCountCONT(), getItemsCONT(), RE::FormType::Container);
					}
					if (getItemCountACTI() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text(ICON_MDI_COGS " 可互动(%d)", getItemCountACTI());

						buildItemInfo(getItemCountACTI(), getItemsACTI(), RE::FormType::Activator);
					}
					if (getItemCount() > 0) {
						ImGui::TableNextColumn();
						ImGui::Text("其他：");
						buildItemInfo(getItemCount(), getItems(), RE::FormType::None);
					}

					ImGui::TableNextColumn();
					ImGui::Checkbox("更多", &show_items_window_settings);
					if (show_items_window_settings) {
						ImGui::SameLine(0.0f, 9.1f * ImGui::GetTextLineHeightWithSpacing());

						if (ImGui::Button(ICON_MDI_CONTENT_SAVE " 保存设置##2")) {
							auto colorPlotHistogram = style.Colors[ImGuiCol_PlotHistogram];
							colorPlotHistogramX = colorPlotHistogram.x;
							colorPlotHistogramY = colorPlotHistogram.y;
							colorPlotHistogramZ = colorPlotHistogram.z;
							colorPlotHistogramW = colorPlotHistogram.w;
							setting::save_settings();
						}

						{
							ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;

							ImGui::BeginChild("childItemsSetting", ImVec2(ImGui::GetTextLineHeightWithSpacing() * 15, ImGui::GetTextLineHeightWithSpacing() * (14.5f)), true, window_flags);

							if (ImGui::BeginTable("tableItemsSetting", 2)) {
								ImGui::TableNextColumn();
								ImGui::Checkbox("显示FORMID", &show_items_window_formid);
								ImGui::TableNextColumn();
								ImGui::Checkbox("显示REFID", &show_items_window_refid);
								ImGui::TableNextColumn();
								ImGui::Checkbox("显示方位", &show_items_window_direction);
								ImGui::TableNextColumn();
								ImGui::Checkbox("显示来自MOD", &show_items_window_file);
								ImGui::TableNextColumn();
								ImGui::Checkbox("显示无模型物品", &show_items_window_3D);
								ImGui::EndTable();
							}

							ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
							ImGui::DragInt("显示范围(本地)", &show_items_window_auto_dis_local, 1, 10, 2000, "%d米");
							ImGui::DragInt("显示范围(天际)", &show_items_window_auto_dis_skyrim, 1, 20, 10000, "%d米");
							ImGui::PopItemWidth();

							ImGui::Checkbox("忽略项目", &show_items_window_ignore);
							if (show_items_window_ignore) {
								static ImGuiTableFlags flagsItem =
									ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;

								const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
								if (ImGui::BeginTable("tableItemIngore", 3, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 12, TEXT_BASE_HEIGHT * 6), 0.0f)) {
									ImGui::TableSetupColumn("FORMID", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_1);
									ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 60.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_2);
									ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_3);

									ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
									ImGui::TableHeadersRow();

									int deleteFormId = 0;

									ImGuiListClipper clipper;
									clipper.Begin(excludeForms.size());
									while (clipper.Step())
										for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
											ExcludeForm& item = excludeForms[row_n];
											ImGui::PushID(item.formId + 0x2000000);
											ImGui::TableNextRow();
											ImGui::TableNextColumn();
											ImGui::Text("%08X", item.formId);
											ImGui::TableNextColumn();
											ImGui::Text("%s", item.name.c_str());
											ImGui::TableNextColumn();
											//ImGui::Text("%s", item.formTypeStr.c_str());

											if (ImGui::SmallButton(ICON_MDI_CLOSE)) {
												deleteFormId = item.formId;
											}

											ImGui::PopID();
										}
									ImGui::EndTable();
									if (deleteFormId) {
										excludeFormIds.erase(deleteFormId);
										excludeForms.erase(std::remove_if(excludeForms.begin(), excludeForms.end(),
															   [&deleteFormId](const ExcludeForm& x) {
																   return x.formId == deleteFormId;
															   }),
											excludeForms.end());
									}
								}
							}

							if (ImGui::TreeNodeEx(ICON_MDI_AUTORENEW " 自动拾取", ImGuiTreeNodeFlags_DefaultOpen)) {
								ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
								ImGui::DragInt("拾取范围", &show_items_window_auto_dis, 1, 1, 100, "%d米");
								ImGui::DragInt("拾取频率", &refresh_time_auto, 1, 1, 10, "%d秒");
								ImGui::DragInt("拾取数量", &show_items_window_auto_every_max, 1, 1, 10, "%d个");
								ImGui::PopItemWidth();
								ImGui::Checkbox("拾取提示", &show_items_window_auto_notification);

								if (ImGui::TreeNodeEx(ICON_MDI_HUMAN_MALE " 尸体物品过滤", ImGuiTreeNodeFlags_DefaultOpen)) {
									if (ImGui::BeginTable("tableItemsSettingACHR", 3)) {
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_SWORD "武器", &show_items_window_auto_achr_weap);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_SHIELD_HALF_FULL "装备", &show_items_window_auto_achr_armo);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_ARROW_PROJECTILE "弹药", &show_items_window_auto_achr_ammo);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_BOTTLE_TONIC_PLUS_OUTLINE "药水", &show_items_window_auto_achr_alch);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_FOOD_DRUMSTICK "食物", &show_items_window_auto_achr_food);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_SOURCE_BRANCH "材料", &show_items_window_auto_achr_ingr);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_CARDS_DIAMOND "灵魂石", &show_items_window_auto_achr_sgem);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_CASH "金钱", &show_items_window_auto_achr_gold);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_SCRIPT_TEXT "卷轴", &show_items_window_auto_achr_scrl);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_KEY "钥匙", &show_items_window_auto_achr_keym);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_DIAMOND_STONE "宝石", &show_items_window_auto_achr_ston);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_ANVIL "矿锭", &show_items_window_auto_achr_anvi);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_LOCK_OPEN "开锁器", &show_items_window_auto_achr_lock);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_BOX_CUTTER "兽皮", &show_items_window_auto_achr_anhd);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_RABBIT "战利品", &show_items_window_auto_achr_anpa);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_TOOLS "工具", &show_items_window_auto_achr_tool);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_PACKAGE_VARIANT_CLOSED "杂项", &show_items_window_auto_achr_misc);
										ImGui::EndTable();
									}

									ImGui::TreePop();
								}

								if (ImGui::TreeNodeEx(ICON_MDI_ARCHIVE_OUTLINE " 容器物品过滤", ImGuiTreeNodeFlags_DefaultOpen)) {
									if (ImGui::BeginTable("tableItemsSettingCONT", 3)) {
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_SWORD "武器", &show_items_window_auto_cont_weap);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_SHIELD_HALF_FULL "装备", &show_items_window_auto_cont_armo);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_ARROW_PROJECTILE "弹药", &show_items_window_auto_cont_ammo);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_BOTTLE_TONIC_PLUS_OUTLINE "药水", &show_items_window_auto_cont_alch);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_FOOD_DRUMSTICK "食物", &show_items_window_auto_cont_food);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_SOURCE_BRANCH "材料", &show_items_window_auto_cont_ingr);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_CARDS_DIAMOND "灵魂石", &show_items_window_auto_cont_sgem);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_CASH "金钱", &show_items_window_auto_cont_gold);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_SCRIPT_TEXT "卷轴", &show_items_window_auto_cont_scrl);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_KEY "钥匙", &show_items_window_auto_cont_keym);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_DIAMOND_STONE "宝石", &show_items_window_auto_cont_ston);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_ANVIL "矿锭", &show_items_window_auto_cont_anvi);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_LOCK_OPEN "开锁器", &show_items_window_auto_cont_lock);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_BOX_CUTTER "兽皮", &show_items_window_auto_cont_anhd);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_RABBIT "战利品", &show_items_window_auto_cont_anpa);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_TOOLS "工具", &show_items_window_auto_cont_tool);
										ImGui::TableNextColumn();
										ImGui::Checkbox(ICON_MDI_PACKAGE_VARIANT_CLOSED "杂项", &show_items_window_auto_cont_misc);
										ImGui::EndTable();
									}

									ImGui::TreePop();
								}

								if (ImGui::TreeNodeEx(ICON_MDI_SWORD " 武器过滤", ImGuiTreeNodeFlags_DefaultOpen)) {
									ImGui::Checkbox("只拾取附魔武器", &show_items_window_auto_weap_enchant);
									ImGui::Checkbox("设置拾取价值", &show_items_window_auto_weap_price);
									ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
									if (show_items_window_auto_weap_price) {
										ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
										ImGui::DragInt("##设置价格", &show_items_window_auto_weap_price_value, 1, 100, 10000, " >%d");
									}
									ImGui::PopItemWidth();

									ImGui::Checkbox("设置拾取价重比", &show_items_window_auto_weap_priceweight);
									ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
									if (show_items_window_auto_weap_priceweight) {
										ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
										ImGui::DragInt("##设置价重比", &show_items_window_auto_weap_priceweight_value, 1, 2, 500, " >%d");
									}
									ImGui::PopItemWidth();

									ImGui::TreePop();
								}

								if (ImGui::TreeNodeEx(ICON_MDI_SHIELD_HALF_FULL " 装备过滤", ImGuiTreeNodeFlags_DefaultOpen)) {
									ImGui::Checkbox("只拾取附魔装备", &show_items_window_auto_armo_enchant);

									ImGui::Checkbox("设置拾取价值##2", &show_items_window_auto_armo_price);
									ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
									if (show_items_window_auto_armo_price) {
										ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
										ImGui::DragInt("##设置价格2", &show_items_window_auto_armo_price_value, 1, 100, 10000, " >%d");
									}
									ImGui::PopItemWidth();

									ImGui::Checkbox("设置拾取价重比##2", &show_items_window_auto_armo_priceweight);
									ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
									if (show_items_window_auto_armo_priceweight) {
										ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
										ImGui::DragInt("##设置价重比2", &show_items_window_auto_armo_priceweight_value, 1, 2, 500, " >%d");
									}
									ImGui::PopItemWidth();

									ImGui::TreePop();
								}

								ImGui::Checkbox("地点过滤", &show_items_window_auto_ignore);
								if (show_items_window_auto_ignore) {
									ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
									if (ImGui::SmallButton("排除当前位置")) {
										auto player = RE::PlayerCharacter::GetSingleton();
										auto currentLocation = player->currentLocation;
										RE::FormID formid = 0;
										std::string name = "天际";
										if (currentLocation) {
											formid = currentLocation->GetFormID();
											name = currentLocation->GetFullName();
										}
										bool exist = false;
										for (const auto& excludeForm : excludeLocationForms) {
											if (excludeForm.formId == formid) {
												exist = true;
												break;
											}
										}
										if (!exist) {
											excludeLocationForms.push_back({ formid, name, "" });
										}
										excludeLocationFormIds.insert(formid);
									}
									ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
									ImGui::Text(ICON_MDI_MAP_MARKER_RADIUS " %s", playerInfo.location.c_str());
									static ImGuiTableFlags flagsItem =
										ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;

									const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
									if (ImGui::BeginTable("tableItemLocationIngore", 3, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 12, TEXT_BASE_HEIGHT * 6), 0.0f)) {
										ImGui::TableSetupColumn("FORMID", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_1);
										ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 60.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_2);
										ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_3);

										ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
										ImGui::TableHeadersRow();

										int deleteFormId = 0;

										ImGuiListClipper clipper;
										clipper.Begin(excludeLocationForms.size());
										while (clipper.Step())
											for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
												ExcludeForm& item = excludeLocationForms[row_n];
												ImGui::PushID(item.formId + 0x3000000);
												ImGui::TableNextRow();
												ImGui::TableNextColumn();
												ImGui::Text("%08X", item.formId);
												ImGui::TableNextColumn();
												ImGui::Text("%s", item.name.c_str());
												ImGui::TableNextColumn();
												//ImGui::Text("%s", item.formTypeStr.c_str());

												if (ImGui::SmallButton(ICON_MDI_CLOSE)) {
													deleteFormId = item.formId == 0 ? -1 : item.formId;
												}

												ImGui::PopID();
											}
										ImGui::EndTable();
										if (deleteFormId != 0) {
											if (deleteFormId == -1) {
												deleteFormId = 0;
											}
											excludeLocationFormIds.erase(deleteFormId);
											excludeLocationForms.erase(std::remove_if(excludeLocationForms.begin(), excludeLocationForms.end(),
																		   [&deleteFormId](const ExcludeForm& x) {
																			   return x.formId == deleteFormId;
																		   }),
												excludeLocationForms.end());
										}
									}
								}

								ImGui::TreePop();
							}

#ifndef NDEBUG

							if (ImGui::TreeNodeEx(ICON_MDI_HOME_MODERN " 龙裔艺术馆", ImGuiTreeNodeFlags_DefaultOpen)) {
								static ImGuiTableFlags flagsItem =
									ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;
								const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
								if (ImGui::BeginTable("tableLotd", 2, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 12, TEXT_BASE_HEIGHT * 6), 0.0f)) {
									ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthFixed, 80, 1);
									ImGui::TableSetupColumn("数量", ImGuiTableColumnFlags_WidthFixed, 30, 2);
									ImGui::TableSetupScrollFreeze(0, 1);
									ImGui::TableHeadersRow();

									ImGuiListClipper clipper;
									clipper.Begin(lotd::lists.size());
									while (clipper.Step())
										for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
											lotd::List& item = lotd::lists[row_n];
											ImGui::PushID(item.name.c_str());
											ImGui::TableNextRow();
											ImGui::TableNextColumn();
											ImGui::Text("%s", item.name.c_str());
											ImGui::TableNextColumn();
											ImGui::Text("%d-%d-%d_%d-%d-%d", item.size, item.sizeACTI, item.sizeFLST, item.size2, item.sizeACTI2, item.sizeFLST2);
											ImGui::PopID();
										}
									ImGui::EndTable();
								}

								//	ImGui::Checkbox("艺术馆物品板块", &show_items_window_gallery);

								//	ImGui::Text("MOD清单 [已识别%d(%d)件物品,%d(%d)个MOD]", galleryItemCount, galleryItemTotalCount, galleryModCount, galleryModTotalCount);

								//	static ImGuiTableFlags flagsItem =
								//		ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;
								//	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
								//	if (ImGui::BeginTable("tableGalleryMod", 4, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 12, TEXT_BASE_HEIGHT * 6), 0.0f)) {
								//		ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 30, PlayerInfoColumnID_1);
								//		ImGui::TableSetupColumn("MOD", ImGuiTableColumnFlags_WidthFixed, 80, PlayerInfoColumnID_2);
								//		ImGui::TableSetupColumn("数量", ImGuiTableColumnFlags_WidthFixed, 40, PlayerInfoColumnID_4);
								//		ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 60, PlayerInfoColumnID_3);

								//		ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
								//		ImGui::TableHeadersRow();

								//		int deleteFormId = 0;

								//		ImGuiListClipper clipper;
								//		clipper.Begin(galleryFormModData.size());
								//		while (clipper.Step())
								//			for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
								//				GalleryModForm item = galleryFormModData[row_n];
								//				ImGui::PushID(item.filename.c_str());
								//				ImGui::TableNextRow();

								//				if (item.compileIndex != -1) {
								//					ImGui::TableNextColumn();
								//					if (item.compileIndex == 0xFE) {
								//						ImGui::Text("%02X:%03X", item.compileIndex, item.smallFileCompileIndex);
								//					} else {
								//						ImGui::Text("%02X", item.compileIndex);
								//					}

								//					ImGui::TableNextColumn();
								//					ImGui::Text("%s", item.filename.c_str());
								//					ImGui::TableNextColumn();
								//					ImGui::Text("%d(%d)", item.itemCount, item.totalItemCount);
								//					ImGui::TableNextColumn();
								//					ImGui::Text("%s", item.name.c_str());
								//				} else {
								//					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
								//					ImGui::TableNextColumn();

								//					ImGui::TableNextColumn();
								//					ImGui::Text("%s", item.filename.c_str());
								//					ImGui::TableNextColumn();
								//					ImGui::Text("%d(%d)", item.itemCount, item.totalItemCount);
								//					ImGui::TableNextColumn();
								//					ImGui::Text("%s", item.name.c_str());
								//					ImGui::PopStyleColor();
								//				}

								//				ImGui::PopID();
								//			}
								//		ImGui::EndTable();
								//	}

								//	static char str0[128] = "93F97";
								//	if (ImGui::Button(ICON_MDI_CONTENT_SAVE " 生成配置文件")) {
								//		std::string formidstr = str0;
								//		auto form = RE::TESForm::LookupByID(std::stoi(formidstr, 0, 16));
								//		if (form) {
								//			if (form->Is(RE::FormType::Reference)) {
								//				auto reff = form->AsReference();
								//				if (reff) {
								//					auto baseObj = reff->GetBaseObject();
								//					if (baseObj) {
								//						if (baseObj->GetFormType() == RE::FormType::Container) {
								//							auto inv = reff->GetInventory(CanDisplay);

								//							std::vector<setting::GalleryDataGen> galleryList;
								//							for (auto& [obj, data] : inv) {
								//								auto& [count, entry] = data;
								//								if (count > 0 && entry) {
								//									bool flag = false;
								//									// 检查一下mod是否存在
								//									for (auto& data : galleryList) {
								//										if (data.filename == obj->GetFile(0)->fileName) {
								//											// 存在直接加
								//											flag = true;

								//											int formid = obj->GetFormID();
								//											if (obj->GetFile(0)->IsLight()) {
								//												formid &= 0x00000FFF;
								//											} else {
								//												formid &= 0x00FFFFFF;
								//											}
								//											char buf[80];
								//											snprintf(buf, 80, "%08X", formid);
								//											data.formids.push_back(buf);
								//											break;
								//										}
								//									}
								//									if (!flag) {
								//										setting::GalleryDataGen data;
								//										data.filename = obj->GetFile(0)->fileName;
								//										data.name = obj->GetFile(0)->fileName;

								//										int formid = obj->GetFormID();
								//										if (obj->GetFile(0)->IsLight()) {
								//											formid &= 0x00000FFF;
								//										} else {
								//											formid &= 0x00FFFFFF;
								//										}
								//										char buf[80];
								//										snprintf(buf, 80, "%08X", formid);
								//										data.formids.push_back(buf);
								//										galleryList.push_back(data);
								//									}
								//								}
								//							}

								//							std::filesystem::path settings_path = "data\\skse\\plugins\\ItemFinderPlus-GalleryList-Gen.json";

								//							nlohmann::json arr = nlohmann::json::array();

								//							for (const auto& data : galleryList) {
								//								nlohmann::json obj = nlohmann::json::object();
								//								obj["filename"] = data.filename;
								//								obj["name"] = data.name;
								//								nlohmann::json arr2 = nlohmann::json::array();
								//								for (const auto& formid : data.formids) {
								//									arr2.push_back(formid);
								//								}
								//								obj["formids"] = arr2;
								//								arr.push_back(obj);
								//							}

								//							std::ofstream o(settings_path);
								//							o << std::setw(4) << arr << std::endl;
								//						}
								//					}
								//				}
								//			}
								//		}
								//	}
								//	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
								//	ImGui::InputText("ID:", str0, IM_ARRAYSIZE(str0));

								ImGui::TreePop();
							}

#endif
							ImGui::EndChild();
						}
					}
				}

				ImGui::EndTable();
			}

			ImGui::End();
		}

		//if (show_crosshair) {
		//	ImGui::Begin("准星信息", nullptr, window_flags);
		//	RE::TESObjectREFR* _cachedRef = BSTCrosshairRefEvent::GetSingleton()->GetCrosshairReference();
		//	if (_cachedRef) {
		//		logger::debug("准星");
		//		logger::debug(StringUtil::Utf8ToGbk(_cachedRef->GetDisplayFullName()));
		//		logger::debug(_cachedRef->GetFormID());
		//	}

		//	ImGui::End();
		//}

		if (active) {
			static bool show_demo_window = false;
			static bool show_another_window = false;

			if (show_demo_window)
				ImGui::ShowDemoWindow(&show_demo_window);

			{
				ImGui::Begin("ItemFinderPlus v0.7##0", nullptr, 0);

				ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
				if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
					if (ImGui::BeginTabItem(ICON_MDI_TOOLS " MOD设置", 0, 0)) {
						ImGui::PushItemWidth(ImGui::GetFontSize() * 8);

						static int style_idx = imgui_style_index;
						if (ImGui::Combo("UI主题", &style_idx, theme_items, -1)) {
							imgui_style_index = style_idx;
							buildStyle();
						}

						if (ImGui::Combo("UI字体", &imgui_font_index, font_items, -1)) {
							ImGuiIO& io = ImGui::GetIO();
							io.FontDefault = fonts[imgui_font_index];
						}

						ImGui::DragFloat("UI缩放", &ImGui::GetIO().FontGlobalScale, 0.005f, 0.5f, 1.8f, "%.2f", 1);
						ImGui::Combo("窗口热键", &hotkey, "Insert\0F11\0F12\0Shift+Q\0Alt+Q\0", -1);
						//ImGui::DragInt("数据刷新(ms)", &refresh_time_data, 1, 100, 500, "%d ms");
						ImGui::PopItemWidth();

						if (ImGui::TreeNodeEx(ICON_MDI_TABLE_OF_CONTENTS " HUD", ImGuiTreeNodeFlags_DefaultOpen)) {
							ImGui::Checkbox("背景透明", &no_background);
							ImGui::TreePop();
						}

						if (ImGui::TreeNodeEx(ICON_MDI_TABLE_SEARCH " 物品菜单", ImGuiTreeNodeFlags_DefaultOpen)) {
							ImGui::PushItemWidth(ImGui::GetFontSize() * 8);
							ImGui::Combo("窗口热键", &hotkey2, "Insert\0F11\0F12\0Shift+Q\0Alt+Q\0", -1);
							ImGui::Checkbox("背景透明", &no_background_items);
							ImGui::PopItemWidth();
							ImGui::TreePop();
						}

						ImGui::Separator();

#ifndef NDEBUG
						ImGui::Checkbox("demo", &show_demo_window);
#endif

						if (ImGui::Button(ICON_MDI_CONTENT_SAVE " 保存配置", ImVec2(0, 0))) {
							auto colorPlotHistogram = style.Colors[ImGuiCol_PlotHistogram];
							colorPlotHistogramX = colorPlotHistogram.x;
							colorPlotHistogramY = colorPlotHistogram.y;
							colorPlotHistogramZ = colorPlotHistogram.z;
							colorPlotHistogramW = colorPlotHistogram.w;

							setting::save_settings();
						}
						ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
						ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize("v0.7-beta").x - 3);
						myTextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "v0.7-beta");
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem(ICON_MDI_TABLE_OF_CONTENTS " HUD", 0, 0)) {
						if (ImGui::BeginTable("split", 2)) {
							ImGui::TableNextColumn();
							ImGui::Checkbox("人物基本属性", &show_player_base_info_window);

							if (show_player_base_info_window) {
								if (ImGui::TreeNodeEx("设置##3", ImGuiTreeNodeFlags_DefaultOpen)) {
									ImGui::Checkbox("独立显示", &show_player_base_info_window_sep);
									if (!show_player_base_info_window_sep) {
										ImGui::Checkbox("显示进度条", &flag_process);
										if (flag_process) {
											if (ImGui::ColorEdit4("进度条颜色", &colorProgress.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
												;
										}
									}
									ImGui::TreePop();
								}
							}

							ImGui::TableNextColumn();
							ImGui::TableNextColumn();
							ImGui::Checkbox("人物属性", &show_player_info_window);
							ImGui::TableNextColumn();
							ImGui::Checkbox("人物属性2", &show_player_mod_window);
							ImGui::TableNextColumn();
							ImGui::Checkbox("武器信息", &show_player_weapon_window);
							ImGui::TableNextColumn();
							ImGui::Checkbox("防具信息", &show_player_armor_window);
							ImGui::TableNextColumn();
							ImGui::Checkbox("其他信息", &show_player_debug_window);
							ImGui::TableNextColumn();
							/*ImGui::Checkbox("物品信息(预留)", &show_items_window);*/
							ImGui::TableNextColumn();
							//ImGui::Checkbox("敌人信息", &show_enemy_window);
							ImGui::Checkbox("NPC信息", &show_npc_window);
							if (show_npc_window) {
								if (ImGui::TreeNodeEx("设置##1", ImGuiTreeNodeFlags_DefaultOpen)) {
									ImGui::Checkbox("只显示距离内NPC", &show_npc_window_dis);
									if (show_npc_window_dis) {
										//ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
										ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
										ImGui::DragInt("##距离", &show_npc_window_dis_meter, 1, 10, 100, "%d米内");
										ImGui::PopItemWidth();
									}
									ImGui::Checkbox("显示FORMID", &show_npc_window_formid);
									ImGui::Checkbox("显示方向和距离", &show_npc_window_direction);
									ImGui::Checkbox("死亡不显示", &show_npc_window_dead_hidden);

									ImGui::TreePop();
								}
							}
							/*ImGui::TableNextColumn();
								ImGui::Checkbox("物品栏信息", &show_inv_window);
								if (show_inv_window) {
									if (ImGui::TreeNodeEx("设置##2", ImGuiTreeNodeFlags_DefaultOpen)) {
										ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
										ImGui::DragInt("显示数据", &show_inv_window_height, 1, 15, 80, "%d条");
										ImGui::Checkbox("激活菜单时显示", &show_inv_window_active);
										ImGui::PopItemWidth();
										ImGui::TreePop();
									}
								}*/

							ImGui::EndTable();
						}

						ImGui::Separator();
						if (ImGui::BeginTable("split3", 3)) {
							ImGui::TableNextColumn();
							ImGui::Checkbox("不显示标题", &no_titlebar);
							ImGui::TableNextColumn();
							ImGui::Checkbox("不允许缩放", &no_resize);
							ImGui::TableNextColumn();
							ImGui::Checkbox("不允许折叠", &no_collapse);
							ImGui::TableNextColumn();
							ImGui::Checkbox("不显示背景", &no_background);
							ImGui::TableNextColumn();
							ImGui::Checkbox("窗体边框", &window_border);
							ImGui::TableNextColumn();
							ImGui::Checkbox("控件边框", &frame_border);
							ImGui::TableNextColumn();
							ImGui::Checkbox("是否窗口自适应", &auto_resize);
							ImGui::EndTable();
						}

						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem(ICON_MDI_ACCOUNT_EDIT " 其他", 0, 0)) {
						RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

						ImGui::Checkbox("是否自动卸除箭袋", &auto_remove_ammo);
						if (auto_remove_ammo) {
							ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
							ImGui::Checkbox("是否记住箭矢类型", &remember_last_ammo);
						}

						ImGui::Separator();

						if (ImGui::TreeNodeEx("修改属性一", ImGuiTreeNodeFlags_DefaultOpen)) {
							if (ImGui::BeginTable("split", 2, 0)) {
								ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale);
								ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.0f);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("修改项:");

								ImGui::TableNextColumn();
								ImGui::Combo("##cbActorValue", &stateMod_selectIndex, actorValues, ((int)(sizeof(actorValues) / sizeof(*(actorValues)))), 6);
								if (player) {
									if (stateMod_selectIndex >= 0) {
										stateMod_nowValue = player->GetActorValue(actorValuesIndex[stateMod_selectIndex]);
									}
								}
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("当前值(总):");
								ImGui::TableNextColumn();
								ImGui::Text("[%.1f]", stateMod_nowValue);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								if (setvalue_Permanent) {
									ImGui::Text("增加值:");
								} else {
									ImGui::Text("修改值(临时):");
								}
								ImGui::TableNextColumn();
								if (stateMod_lastSelectIndex != stateMod_selectIndex) {
									stateMod_newValue = stateMod_nowValue;
									stateMod_lastSelectIndex = stateMod_selectIndex;
								}
								ImGui::InputFloat("##newValue", &stateMod_newValue, 10, 100, "%.1f", 0);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								if (ImGui::Button(ICON_MDI_CHECK " 确认修改")) {
									if (player) {
										if (stateMod_selectIndex >= 0) {
											if (setvalue_Permanent) {
												player->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kPermanent, actorValuesIndex[stateMod_selectIndex], stateMod_newValue);
											} else {
												player->SetActorValue(actorValuesIndex[stateMod_selectIndex], stateMod_newValue);
											}
										}
									}
								}
								ImGui::TableNextColumn();
								ImGui::Checkbox("永久", &setvalue_Permanent);

								ImGui::EndTable();
							}

							ImGui::TreePop();
						}

						ImGui::Separator();

						if (ImGui::TreeNodeEx("修改属性二(永久)", ImGuiTreeNodeFlags_DefaultOpen)) {
							if (ImGui::BeginTable("split", 2, 0)) {
								ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale);
								ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.0f);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();

								ImGui::Text("修改项:");
								ImGui::TableNextColumn();
								ImGui::Combo("##cbActorValuePer", &statePerMod_selectIndex, perActorValues, ((int)(sizeof(perActorValues) / sizeof(*(perActorValues)))), 6);
								if (player) {
									if (statePerMod_selectIndex >= 0) {
										statePerMod_nowValue = player->GetActorValue(perActorValuesIndex[statePerMod_selectIndex]);
									}
								}

								ImGui::TableNextRow();
								ImGui::TableNextColumn();

								ImGui::Text("当前值(总):");
								ImGui::TableNextColumn();
								ImGui::Text("[%.1f]", statePerMod_nowValue);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("修改值(基础):");
								ImGui::TableNextColumn();
								if (statePerMod_lastSelectIndex != statePerMod_selectIndex) {
									statePerMod_newValue = statePerMod_nowValue;
									statePerMod_lastSelectIndex = statePerMod_selectIndex;
								}
								ImGui::InputFloat("##newValuePer", &statePerMod_newValue, 10, 100, "%.1f", 0);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								if (ImGui::Button(ICON_MDI_CHECK " 确认修改")) {
									if (player) {
										if (statePerMod_selectIndex >= 0) {
											player->SetActorValue(perActorValuesIndex[statePerMod_selectIndex], statePerMod_newValue);
										}
									}
								}

								ImGui::EndTable();
							}
							ImGui::TreePop();
						}

						ImGui::Separator();

						if (ImGui::TreeNodeEx("常用素材获取", ImGuiTreeNodeFlags_DefaultOpen)) {
							if (ImGui::BeginTable("split", 2, 0)) {
								ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetIO().FontGlobalScale);
								ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.0f);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("获取项:");
								ImGui::TableNextColumn();
								ImGui::Combo("##cbInvGet", &getInv_selectIndex, invValues, ((int)(sizeof(invValues) / sizeof(*(invValues)))), 6);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("数量:");
								ImGui::TableNextColumn();
								ImGui::InputInt("##InvCount", &getInv_nowValue, 10, 9999);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								if (ImGui::Button(ICON_MDI_CHECK " 确认获取")) {
									std::string commandStr = "player.additem ";
									commandStr.append(invValuesCode[getInv_selectIndex]);
									commandStr.append(" ");
									commandStr.append(std::to_string(getInv_nowValue));
									ScriptUtil::ExecuteCommand(commandStr);
								}

								ImGui::EndTable();
							}
							ImGui::TreePop();
						}

						ImGui::Separator();

						if (ImGui::TreeNodeEx("天气设置", ImGuiTreeNodeFlags_DefaultOpen)) {
							auto sky = RE::Sky::GetSingleton();
							auto calendar = RE::Calendar::GetSingleton();

							ImGui::Text(ICON_MDI_CLOCK_OUTLINE " 当前时间:");
							if (calendar) {
								auto time = &(calendar->gameHour->value);
								if (time) {
									ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
									ImGui::PushItemWidth(ImGui::GetFontSize() * 4);
									ImGui::SliderFloat("##Time", time, 0.0f, 24.f, "%.2f");
									ImGui::PopItemWidth();
								}
							}
							if (sky) {
								auto weather = sky->currentWeather;
								if (weather) {
									ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
									auto flags = weather->data.flags;

									if (flags.any(RE::TESWeather::WeatherDataFlag::kPleasant)) {
										ImGui::Text(ICON_MDI_WEATHER_SUNNY " 天气:%s", weather->GetFormEditorID());
									} else if (flags.any(RE::TESWeather::WeatherDataFlag::kCloudy)) {
										ImGui::Text(ICON_MDI_WEATHER_CLOUDY " 天气:%s", weather->GetFormEditorID());
									} else if (flags.any(RE::TESWeather::WeatherDataFlag::kRainy)) {
										ImGui::Text(ICON_MDI_WEATHER_POURING " 天气:%s", weather->GetFormEditorID());
									} else if (flags.any(RE::TESWeather::WeatherDataFlag::kSnow)) {
										ImGui::Text(ICON_MDI_WEATHER_SNOWY " 天气:%s", weather->GetFormEditorID());
									} else if (flags.any(RE::TESWeather::WeatherDataFlag::kPermAurora)) {
										ImGui::Text(ICON_MDI_AURORA " 天气:%s", weather->GetFormEditorID());
									} else if (flags.any(RE::TESWeather::WeatherDataFlag::kAuroraFollowsSun)) {
										ImGui::Text(ICON_MDI_WEATHER_SUNNY " 天气:%s", weather->GetFormEditorID());
									} else {
										ImGui::Text(ICON_MDI_WEATHER_PARTLY_CLOUDY " 天气:%s", weather->GetFormEditorID());
									}
								}
							}

							static ImGuiTableFlags flagsItem =
								ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;

							const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
							if (ImGui::BeginTable("tableWeather", 4, flagsItem, ImVec2(TEXT_BASE_HEIGHT * 16, TEXT_BASE_HEIGHT * 9), 0.0f)) {
								ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_3);
								ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_4);
								ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 130.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_2);
								ImGui::TableSetupColumn("FORMID", ImGuiTableColumnFlags_WidthFixed, 90.0f * ImGui::GetIO().FontGlobalScale, PlayerInfoColumnID_1);

								ImGui::TableSetupScrollFreeze(0, 1);  // Make row always visible
								ImGui::TableHeadersRow();

								ImGuiListClipper clipper;
								clipper.Begin(weatherForms.size());
								while (clipper.Step())
									for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
										WeatherForm& item = weatherForms[row_n];
										ImGui::PushID(item.formId + 0x1000000);
										ImGui::TableNextRow();

										ImGui::TableNextColumn();
										if (currentWeather != item.formId) {
											if (ImGui::SmallButton(ICON_MDI_CHECK)) {
												auto sky = RE::Sky::GetSingleton();
												if (sky) {
													auto weather = RE::TESForm::LookupByID<RE::TESWeather>(item.formId);
													if (weather) {
														sky->ForceWeather(weather, true);
														currentWeather = item.formId;
													}
												}
											}
										}
										ImGui::TableNextColumn();
										switch (item.flag) {
										case RE::TESWeather::WeatherDataFlag::kPleasant:
											ImGui::Text(ICON_MDI_WEATHER_SUNNY);
											break;
										case RE::TESWeather::WeatherDataFlag::kCloudy:
											ImGui::Text(ICON_MDI_WEATHER_CLOUDY);
											break;
										case RE::TESWeather::WeatherDataFlag::kRainy:
											ImGui::Text(ICON_MDI_WEATHER_POURING);
											break;
										case RE::TESWeather::WeatherDataFlag::kSnow:
											ImGui::Text(ICON_MDI_WEATHER_SNOWY);
											break;
										case RE::TESWeather::WeatherDataFlag::kPermAurora:
											ImGui::Text(ICON_MDI_AURORA);
											break;
										case RE::TESWeather::WeatherDataFlag::kAuroraFollowsSun:
											ImGui::Text(ICON_MDI_WEATHER_SUNNY);
											break;
										default:
											ImGui::Text(ICON_MDI_WEATHER_PARTLY_CLOUDY);
											break;
										}

										ImGui::TableNextColumn();
										ImGui::Text("%s", item.editorId.c_str());
										ImGui::TableNextColumn();
										ImGui::Text("%08X", item.formId);

										ImGui::PopID();
									}
								ImGui::EndTable();
							}

							ImGui::TreePop();
						}
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}

				ImGui::End();
			}
		}
	}

}
