# AiWeapon 무기 에셋 제작 기록

**상태:** 1차 제작 후 분리/부유 부품 정리 및 재임포트 완료  
**대상 폴더:** `/Game/FieldMaker/Weapons/AiWeapon`  
**Blender 원본:** `SourceArt/Blender/AiWeapon_23.blend`  
**FBX 내보내기:** `SourceArt/Blender/AiWeaponFBX/`  
**제작 목록:** `SourceArt/Blender/AiWeapon_manifest.json`

## 결과물

요청한 분류별 수량으로 무기 메시 23개를 만들었습니다. 재질을 정리하기 위해 별도 `Materials` 폴더도 만들었습니다. 기존 무기 원본은 보존했습니다.

| 분류 | 수량 | 에셋 |
|---|---:|---|
| 권총 | 5 | Glock18, USPS, P250, FiveSeven, DesertEagle |
| 기관단총 | 5 | MAC10, UMP45, MP5SD, MP7, P90 |
| 라이플 | 3 | AK47, AR70, AR15 |
| 저격소총 | 2 | Ghoul, AWP |
| 중화기 | 3 | Nova, Negev, M249 |
| 투척류 | 5 | HE, Flashbang, Smoke, Incendiary, Decoy |
| **합계** | **23** | |

각 메시의 Blender 기반과 원본 메시 연결은 `AiWeapon_manifest.json`에 기록했습니다. 현재 23개는 프로젝트 기존 메시와 Blender 형상을 활용한 플레이용 세트이며, 23개 모두가 서로 다른 제조사 원본 모델은 아닙니다. 2026-11-13 정리 과정에서 총몸에서 떨어져 보이던 일반 박스 장식을 제거했습니다. 특히 Negev에 잘못 추가되어 총 아래에 떠 있던 타원형 드럼을 없앴고, 실제 총구 축과 맞지 않던 권총·SMG 장식도 제거했습니다. USPS의 소음기만 총열 높이에 맞춰 유지했습니다.

## 메시별 기반과 특징

| 무기 | 기반 메시 | 형태에서 살린 특징 |
|---|---|---|
| Glock18 | `SM_FM_PolicePistol_Fitted` | 권총 기반 메시 유지. 슬라이드 톱니와 프레임은 원본 메시 형상 |
| USPS | `SM_FM_PolicePistol_Fitted` | 권총 기반에 총열 축으로 정렬한 소음기 형상 추가 |
| P250 | `SM_FM_PolicePistol_Fitted` | 현재 Glock과 같은 기반 메시. 고유 P250 형상은 아직 필요 |
| FiveSeven | `SM_FM_PolicePistol_Fitted` | 현재 Glock과 같은 기반 메시. 고유 Five-seveN 형상은 아직 필요 |
| DesertEagle | `SM_FM_PolicePistol_Fitted` | 현재 Glock과 같은 기반 메시. 대형 Desert Eagle 형상은 아직 필요 |
| MAC10 | `SM_FM_SMG11_Fitted` | SMG11 기반 메시 사용. MAC-10 고유 외형을 대체할 별도 원본이 필요 |
| UMP45 | `SM_FM_MP5_Fitted` | MP5 기반 메시 사용. UMP45 고유 외형은 아직 아님 |
| MP5SD | `SM_FM_MP5_Fitted` | MP5 기반 메시 사용. MP5SD 전용 소음기 형상은 아직 필요 |
| MP7 | `SM_FM_SMG11_Fitted` | SMG11 기반 메시 사용. MP7 고유 PDW 외형은 아직 아님 |
| P90 | `SM_FM_SMG11_Fitted` | SMG11 기반 메시 사용. P90의 불펍/상부 탄창 메시가 아직 필요 |
| AK47 | `SM_FM_KA47_Fitted` | 곡선 탄창, 프레스 리시버, 목재 부품 |
| AR70 | `SM_FM_AR70_Fitted` | AR70 원본의 상부 리시버와 스톡 실루엣 |
| AR15 | `SM_FM_AR15_Fitted` | AR 계열 버퍼 스톡, 상부 레일/조준기, 분리형 탄창 |
| Ghoul | `SM_FM_Ghoul_Fitted` | 원본 정밀소총의 긴 총열과 조준경 실루엣 |
| AWP | `SM_FM_AWP_Fitted` | 볼트액션 정밀소총의 긴 총열과 대형 조준경 |
| Nova | `SM_FM_Shotgun_Fitted` | 펌프 손잡이, 관형 탄창, 링 조준기 |
| Negev | `SM_FM_M249_Fitted` | M249 기반 메시. 이전에 추가한 부유 드럼은 제거했고 Negev 전용 형상은 아직 필요 |
| M249 | `SM_FM_M249_Fitted` | M249 기반 원본 메시 유지. 추가 부유 양각대 장식은 제거 |
| HE | `SM_FM_G67_Fitted` | 분절형 몸체, 안전 레버, 고리 |
| Flashbang | Blender 형상 | 원통형 캔스터, 엔드캡, 안전 고리 |
| Smoke | Blender 형상 | 골이 있는 원통 몸체와 구분되는 끝단 |
| Incendiary | Blender 형상 | 짧고 굵은 골형 몸체와 안전 식별 밴드 |
| Decoy | Blender 형상 | 짧은 원통 몸체, 통풍 밴드, 식별 줄무늬 |

## 형태 참고 자료

다음 제조사 자료의 외형 설명을 기준으로 해당 계열의 실루엣을 잡았습니다. 프로젝트의 게임용 변형 이름과 실제 제조 모델 이름이 일치하지 않는 경우에는 원본 에셋을 유지한 채 플레이용 이름으로 분류했습니다.

- [HK MP5 제품군](https://www.heckler-koch.com/en/Products/Military%20and%20Law%20Enforcement/Submachine%20guns/MP5): MP5 계열과 소음기형 외관 참고.
- [HK UMP 공식 페이지](https://www.heckler-koch.com/en/Products/Military%20and%20Law%20Enforcement/Submachine%20guns/UMP): 폴리머 구성, 접이식 스톡, 레일 및 직선 탄창 외관 참고.
- [FN P90 공식 페이지](https://fnherstal.com/en/defence/portable-weapons/fn-p90/): 컴팩트 불펍 비례와 상부 통합 탄창 참고.
- [FN Five-seveN 공식 제품 정보](https://fnherstal.com/en/security/handguns/fn-five-seven-mk3-mrd/): 슬라이드·그립 형태 참고.
- [GLOCK G18 공식 설명](https://eu.glock.com/en/technology/full-auto): G18 선택사격형 구성과 GLOCK 계열 외형 참고.
- [MAC-10 정비 매뉴얼](https://powderandball.com/manuals/mac_m10_mini_sub_machinegun.pdf): MAC M10의 리시버·탄창 하우징·스톡 구성 참고.
- [Benelli Defense 카탈로그](https://www.benellidefense.com/wp-content/uploads/2024/10/Catalogo-Defense.pdf): Nova 펌프 산탄총의 외형 참고.

## 재질과 라이선스 메모

- 새로 만든 단순 재질과 기존 원본의 프로젝트 내 재질 사본을 `AiWeapon/Materials`에 분리했습니다.
- 원본 콘텐츠의 재질이나 메시를 덮어쓰지 않았습니다.
- 외부 마켓플레이스 에셋은 이번 작업에서 구매하거나 내려받지 않았습니다. 결과물은 프로젝트에 이미 있던 에셋과 Blender에서 직접 만든 형상을 활용합니다.
- 실제 무기 제조사의 정확한 도면에 따른 치수 복제 또는 법적 제조사 인증 모델은 아닙니다. 손에 맞는 게임 에셋의 비례와 식별 가능한 특징을 목표로 한 시각 모델입니다.

## 검증 범위와 남은 일

- Unreal Content 폴더에서 `SM_AIW_` 정적 메시 23개를 확인했습니다. 23개 전부의 재질 슬롯이 `/Game/FieldMaker/Weapons/AiWeapon/Materials` 내부 재질을 참조하며, `WorldGridMaterial` 참조는 0개입니다.
- Blender 원본 `.blend`, 내보낸 FBX 23개, 제작 목록 JSON을 확인했습니다.
- MAC10, MP7, P90 스케일을 교정했습니다. 이번 정리에서는 권총·SMG·중화기의 위치가 맞지 않던 추가 장식을 제거하고 13개 메시를 재임포트했습니다. 에디터에서 Glock18, USPS, MAC10, Negev 메시를 열어 모양과 재질을 확인했습니다.
- 권총 5종 중 Glock18/USPS/P250/FiveSeven/DesertEagle은 아직 같은 PolicePistol 기반입니다. SMG 일부도 SMG11 또는 MP5 기반을 공유합니다. 따라서 부유 부품 문제는 고쳤지만, 이 총기들의 고유 실루엣까지 완성된 상태는 아닙니다. 정확한 구분을 위해서는 별도 게임용 메시를 구하거나 Blender에서 총별 몸체 형상을 새로 모델링해야 합니다.
- 모든 무기의 손 소켓 장착, 1인칭/3인칭 애니메이션, 재장전 중 탄창 분리, 게임 플레이에서의 최종 손 자세는 이번 메시 에디터 확인 범위에 포함되지 않았습니다.
