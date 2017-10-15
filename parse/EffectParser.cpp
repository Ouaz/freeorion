#include "EffectParserImpl.h"

#include "EffectParser1.h"
#include "EffectParser2.h"
#include "EffectParser3.h"
#include "EffectParser4.h"
#include "EffectParser5.h"

#include "../universe/Condition.h"
#include "../universe/Effect.h"

#include <boost/spirit/include/phoenix.hpp>

namespace {
    /** This constructor avoids phoenix actors return by valur which makes
        difficulties for move only types. */
    Effect::EffectsGroup* new_EffectsGroup(
        Condition::ConditionBase* scope,
        Condition::ConditionBase* activation,
        std::vector<Effect::EffectBase*>& effects_,
        const std::string& accounting_label,
        const std::string& stacking_group,
        int priority,
        const std::string& description)
    {
        std::vector<std::unique_ptr<Effect::EffectBase>> effects;
        for (auto&& effect_ : effects_)
            effects.emplace_back(effect_);

        return new Effect::EffectsGroup(
            std::unique_ptr<Condition::ConditionBase>(scope),
            std::unique_ptr<Condition::ConditionBase>(activation),
            std::move(effects),
            accounting_label, stacking_group, priority, description);
    }

    BOOST_PHOENIX_ADAPT_FUNCTION(Effect::EffectsGroup*, new_EffectsGroup_, new_EffectsGroup, 7)

}

namespace parse {

    /** effects_parser_grammar::Impl holds the rules for
        effects_parser_grammar. */
    struct effects_parser_grammar::Impl {
        Impl (const effects_parser_grammar& effects_parser_grammar,
            const lexer& tok,
            detail::Labeller& labeller,
            const detail::condition_parser_grammar& condition_parser,
            const value_ref_grammar<std::string>& string_grammar
        ) :
            effect_parser_1(tok, labeller, condition_parser, string_grammar),
            effect_parser_2(tok, labeller, condition_parser, string_grammar),
            effect_parser_3(tok, labeller, condition_parser, string_grammar),
            effect_parser_4(tok, effects_parser_grammar, labeller, condition_parser, string_grammar),
            effect_parser_5(tok, effects_parser_grammar, labeller, condition_parser)
        {}

        detail::effect_parser_rules_1 effect_parser_1;
        detail::effect_parser_rules_2 effect_parser_2;
        detail::effect_parser_rules_3 effect_parser_3;
        detail::effect_parser_rules_4 effect_parser_4;
        detail::effect_parser_rules_5 effect_parser_5;
    };

    effects_parser_grammar::effects_parser_grammar(
        const lexer& tok,
        detail::Labeller& labeller,
        const detail::condition_parser_grammar& condition_parser,
        const value_ref_grammar<std::string>& string_grammar
    ) :
        effects_parser_grammar::base_type(start, "effects_parser_grammar"),
        m_impl(new effects_parser_grammar::Impl(*this, tok, labeller, condition_parser, string_grammar))
    {
        start
            = m_impl->effect_parser_1
            | m_impl->effect_parser_2
            | m_impl->effect_parser_3
            | m_impl->effect_parser_4
            | m_impl->effect_parser_5
            ;
        start.name("Effect");
    }

    effects_parser_grammar::~effects_parser_grammar()
    {};

    effects_group_grammar::effects_group_grammar(
        const lexer& tok,
        detail::Labeller& labeller,
        const detail::condition_parser_grammar& condition_parser,
        const parse::value_ref_grammar<std::string>& string_grammar
    ) :
        effects_group_grammar::base_type(start, "effects_group_grammar"),
        effects_grammar(tok, labeller, condition_parser, string_grammar)
    {
        namespace phoenix = boost::phoenix;
        namespace qi = boost::spirit::qi;

        using phoenix::construct;
        using phoenix::new_;
        using phoenix::push_back;

        qi::_1_type _1;
        qi::_a_type _a;
        qi::_b_type _b;
        qi::_c_type _c;
        qi::_d_type _d;
        qi::_e_type _e;
        qi::_f_type _f;
        qi::_g_type _g;
        qi::_val_type _val;
        qi::lit_type lit;
        qi::eps_type eps;

        effects_group
            =   tok.EffectsGroup_
            > -(labeller.rule(Description_token)      > tok.string [ _g = _1 ])
            >   labeller.rule(Scope_token)            > condition_parser [ _a = _1 ]
            > -(labeller.rule(Activation_token)       > condition_parser [ _b = _1 ])
            > -(labeller.rule(StackingGroup_token)    > tok.string [ _c = _1 ])
            > -(labeller.rule(AccountingLabel_token)  > tok.string [ _e = _1 ])
            > ((labeller.rule(Priority_token)         > tok.int_ [ _f = _1 ]) | eps [ _f = 100 ])
            >   labeller.rule(Effects_token)
            >   (
                ('[' > +effects_grammar [ push_back(_d, _1) ] > ']')
                |    effects_grammar [ push_back(_d, _1) ]
                )
            [ _val = new_EffectsGroup_(_a, _b, _d, _e, _c, _f, _g) ]
            ;

        start
            =    ('[' > +effects_group [ push_back(_val, construct<std::shared_ptr<Effect::EffectsGroup>>(_1)) ] > ']')
            |     effects_group [ push_back(_val, construct<std::shared_ptr<Effect::EffectsGroup>>(_1)) ]
            ;

        effects_group.name("EffectsGroup");
        start.name("EffectsGroups");

#if DEBUG_PARSERS
        debug(effects_group);
        debug(start);
#endif
    }
}
