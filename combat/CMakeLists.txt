target_sources(freeorioncommon
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/CombatDamage.h
        ${CMAKE_CURRENT_LIST_DIR}/CombatEvent.h
        ${CMAKE_CURRENT_LIST_DIR}/CombatEvents.h
        ${CMAKE_CURRENT_LIST_DIR}/CombatLogManager.h
        ${CMAKE_CURRENT_LIST_DIR}/CombatSystem.h
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/CombatDamage.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CombatEvent.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CombatEvents.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CombatLogManager.cpp
)

if(BUILD_SERVER)
    target_sources(freeoriond
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/CombatSystem.cpp
    )
endif()

if(BUILD_AI)
    target_sources(freeorionca
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/CombatSystem.cpp
    )
endif()

if(BUILD_CLIENT_GG)
    target_sources(freeorion
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/CombatSystem.cpp
    )
endif()
