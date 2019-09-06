
Backporting patches
===================

This document tries to list all changes that must be made to individual
patches when backporting them to earlier branches of the codebase.

This list is incomplete.

OpenMPT 1.28 / libopenmpt 0.4
-----------------------------

 *  Replace `std::abs` by `mpt::abs`.
 *  Replace `std::byte` by `mpt::byte`.
 *  Replace `std::clamp` by `mpt::clamp`.
 *  Replace `std::data` by `mpt::data`.
 *  Replace `std::gcd` by `mpt::gcd`.
 *  Replace `std::lcm` by `mpt::lcm`.
 *  Replace `std::make_unique` by `mpt::make_unique`.
 *  Replace `std::size` by `mpt::size`.
 *  Replace `if constexpr` by `MPT_CONSTANT_IF`.
 *  Replace `static_assert` by `MPT_STATIC_ASSERT`.
 *  Replace `[[nodiscard]]` by `MPT_NODISCARD`.
 *  Replace `[[fallthrough]]` by `MPT_FALLTHROUGH`.

OpenMPT 1.27 / libopenmpt 0.3 
-----------------------------

 *  Replace string macros with longer versions:
     *  U_("foo") --> MPT_USTRING("foo")
     *  UL_("foo") --> MPT_ULITERAL("foo")
     *  UC_('x') --> MPT_UCHAR('x')
     *  P_("foo") --> MPT_PATHSTRING("foo")
     *  PL_("foo") --> MPT_PATHSTRING_LITERAL("foo")
     *  PC_('x') --> MPT_PATHSTRING_LITERAL('x')
    See <https://bugs.openmpt.org/view.php?id=1107>.
