#ifndef HAYROLL_DEFINESET_HPP
#define HAYROLL_DEFINESET_HPP

#include <optional>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_set>

#include <z3++.h>

#include <spdlog/spdlog.h>

#include "DefinePrefix.hpp"
#include "Util.hpp"

std::optional<std::pair<std::string, std::string>> parseAssignment(const std::string& name) {
    size_t equalsLoc = name.find("=");
    if(equalsLoc == std::string::npos) {
        return {};
    }
    std::string beforeEq = name.substr(0, equalsLoc);
    std::string afterEq = name.substr(equalsLoc);
    return {{beforeEq, afterEq}};
}

namespace Hayroll
{

struct DefineSet
{
    std::unordered_map<std::string, std::optional<int64_t>> defines;

    DefineSet() = default;

    DefineSet(const z3::model & model)
    {
        for (unsigned i = 0; i < model.size(); i++)
        {
            z3::func_decl v = model[i];
            assert(v.arity() == 0); // only constants
            std::string z3VarName = v.name().str();
            std::string prefix = z3VarName.substr(0, 3); // all DEFINE_PREFIX_... are 3 characters long
            std::string name = z3VarName.substr(3);
            z3::expr value = model.get_const_interp(v);
            if (prefix == DEFINE_PREFIX_INTEGER)
            {
                int64_t intValue = value.get_numeral_int64();
                defines.emplace(name, intValue);
            }
            else if (prefix == DEFINE_PREFIX_PRESENT)
            {
                bool boolValue = z3::eq(value, model.ctx().bool_val(true));
                if (boolValue) defines.emplace(name, std::nullopt);
            }
            else if (prefix == DEFINE_PREFIX_EQUALITY)
            {
                bool boolValue = z3::eq(value, model.ctx().bool_val(true));
                if (boolValue) defines.emplace(name, std::nullopt);
                    /*auto parsed = parseAssignment(name);
                    assert(parsed.has_value());
                    auto [var, val] = parsed.value();
                    defines.emplace(var, val);
                    }*/
            }
            else assert(false);
        }
    }

    DefineSet
    (
        const std::unordered_map<std::string, std::optional<int64_t>> & defines
    ) : defines(defines)
    {
    }

    std::vector<std::string> toOptions() const
    {
        std::vector<std::string> options;
        for (const auto & [name, val] : defines)
        {
            if (!val.has_value())
            {
                options.push_back(std::format("-D{}", name));
            }
            else
            {
                options.push_back(std::format("-D{}={}", name, val.value()));
            }
        }
        return options;
    }

    std::string toString() const
    {
        std::stringstream ss;
        for (const std::string & opt : toOptions())
        {
            ss << opt << " ";
        }
        std::string str = ss.str();
        if (!str.empty()) str.pop_back(); // Remove trailing space
        return str;
    }

    bool satisfies(const z3::expr & expr) const
    {
        z3::context & ctx = expr.ctx();
        std::unordered_set<std::string> seen;

        std::function<void(const z3::expr &)> visit = [&seen, &visit](const z3::expr & e)
        {
            if (e.is_app())
            {
                if (e.num_args() == 0)
                {
                    std::string n = e.decl().name().str();
                    if (n.starts_with(DEFINE_PREFIX_PRESENT) || n.starts_with(DEFINE_PREFIX_INTEGER) || n.starts_with(DEFINE_PREFIX_EQUALITY))
                        seen.insert(n);
                }
                for (unsigned i = 0; i < e.num_args(); ++i) visit(e.arg(i));
            }
        };
        visit(expr);

        z3::expr assigns = ctx.bool_val(true);
        for (const std::string & fullName : seen)
        {
            std::string prefix = fullName.substr(0, 3);
            std::string macroName = fullName.substr(3);
            auto it = defines.find(macroName);
            if (prefix == DEFINE_PREFIX_INTEGER)
            {
                int value = 0;
                if (it != defines.end() && it->second.has_value()) value = it->second.value();
                assigns = assigns && (ctx.int_const(fullName.c_str()) == ctx.int_val(value));
            }
            else if (prefix == DEFINE_PREFIX_PRESENT)
            {
                bool defined = (it != defines.end());
                assigns = assigns && (ctx.bool_const(fullName.c_str()) == ctx.bool_val(defined));
            }
            else if (prefix == DEFINE_PREFIX_EQUALITY)
            {
                bool defined = (it != defines.end());
                assigns = assigns && (ctx.bool_const(fullName.c_str()) == ctx.bool_val(defined));
            }
        }

        z3::expr implies = z3::implies(assigns, expr);
        bool ok = z3CheckTautology(implies);
        SPDLOG_TRACE
        (
            "Implication check: set=({}) expr={} assigns={} result={}",
            toString(), expr.to_string(), assigns.to_string(), ok ? "true" : "false"
        );
        return ok;
    }

    static std::string defineSetsToString(const std::vector<DefineSet> & defineSets)
    {
        std::ostringstream oss;
        if (defineSets.empty())
        {
            oss << "// No DefineSets generated\n";
        }
        else
        {
            for (size_t i = 0; i < defineSets.size(); ++i)
            {
                oss << "// DefineSet " << i << "\n";
                oss << defineSets[i].toString() << "\n";
            }
        }
        return oss.str();
    }
};

} // namespace Hayroll

#endif // HAYROLL_DEFINESET_HPP
