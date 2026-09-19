//  Copyright GenericMessagePlugin, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

#include "GMP/GMPBPLib.h"
#include "GMP/GMPArchive.h"
#include "GMP/GMPTickBase.h"
#include "GMP/GMPUtils.h"
#include "GMP/GMPRpcUtils.h"
#include "GMP/GMPThreadUtils.h"
#include "GMP/GMPWorldLocals.h"
#include "Classes/GMPUnion.h"
#if GMP_WITH_DIRECT_SIGNAL
#include "GMP/GMPHubOpt.h"
#endif

namespace GMP
{
class FGMPNetFrameWriter;
class FGMPNetFrameReader;
class FGMPMemoryReader;
class FGMPMemoryWriter;
GMP_API void OnGMPModuleLifetime(FSimpleDelegate Startup, FSimpleDelegate Shutdown = {});
FORCEINLINE void OnGMPTagReady(FSimpleDelegate Callback)
{
	OnGMPModuleLifetime(MoveTemp(Callback));
}
}  // namespace GMP

namespace GMPReflection = GMP::Reflection;
namespace GMPTypeTraits = GMP::TypeTraits;
namespace GMPClass2Name = GMP::Class2Name;
namespace GMPClass2Prop = GMP::Class2Prop;

template<typename T, bool bExactType = true>
using TGMPClass2Name = GMP::TClass2Name<T, bExactType>;

template<typename T, bool bExactType = true>
using TGMPClass2Prop = GMP::TClass2Prop<T, bExactType>;

template<typename T>
using TGMPClassToPropTag = GMP::Class2Prop::TClassToPropTag<T>;

using FGMPSignalsHandle = GMP::FSigHandle;
using IGMPSigSource = GMP::ISigSource;
using FGMPSigSource = GMP::FSigSource;

#define GMP_EXTERNAL_SIGSOURCE(T)                     \
	namespace GMP                                     \
	{                                                 \
		template<>                                    \
		struct TExternalSigSource<T> : std::true_type \
		{                                             \
		};                                            \
	}

using FGMPMessageAddr = FGMPTypedAddr;
using FGMPMessageBody = GMP::FMessageBody;
using FGMPMessageHub = GMP::FMessageHub;

using FGMPHelper = GMP::FMessageUtils;
using FGMPNameSuccession = GMP::FNameSuccession;

template<typename Base, int32 INLINE_SIZE = GMP_FUNCTION_PREDEFINED_INLINE_SIZE>
using TGMPAttachedCallableStore = GMP::TAttachedCallableStore<Base, INLINE_SIZE>;

// none-copyable functions
template<typename TSig>
using TGMPFunction = GMP::TGMPFunction<TSig>;

template<typename TSig>
using TGMPWeakFunction = GMP::TGMPWeakFunction<TSig>;

template<typename TSig>
using TGMPFunctionRef = GMP::TGMPFunctionRef<TSig>;

// FrameTickUtils
template<typename TTask>
using TGMPFrameTickWorldTask = GMP::TGMPFrameTickWorldTask<TTask>;
template<typename F>
decltype(auto) MakeGMPFrameTickWorldTask(const UObject* InObj, F&& LambdaTask, double MaxDurationTime = 0.0)
{
	return GMP::MakeGMPFrameTickWorldTask(InObj, std::forward<F>(LambdaTask), MaxDurationTime);
}

#define GMP_STATIC_PROPERTY(CLASS, NAME) GMPClass2Name::TTraitsUStruct<CLASS>::GetUStruct()->FindPropertyByName(#NAME)
#define GMP_MEMBER_PROPERTY(CLASS, NAME) GMPClass2Name::TTraitsUStruct<CLASS>::GetUStruct()->FindPropertyByName(#NAME)
#define GMP_UFUNCTION_CHECKED(CLASS, NAME) CLASS::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(CLASS, NAME))

//////////////////////////////////////////////////////////////////////////
// Declarative route flattening
//
// A "behavior" key fans out to a set of "op" keys. Declaring a route is pure data -- no forwarder lambda:
//
//     GMP_ROUTE(MSGKEY("Behavior.CloseBackpack"),
//         GMP_FWD(MSGKEY("Op.CloseBagPanel")),
//         GMP_FWD(MSGKEY("Op.CloseTipA")),
//         GMP_FWD(MSGKEY("Op.CloseTipB"))
//     );
//
// At runtime the behavior key is flattened into direct sends to the op keys, so no forwarder runs on the hot
// path. Works identically for local C++ sends, network RPC, and every script backend (see FMessageHub::AddRoute).
// The registrar defers registration through OnGMPModuleLifetime, so this macro is safe at any translation-unit
// scope (static-init) regardless of registration order across files.
namespace GMP::RouteDetail
{
	struct FRouteRegistrar
	{
		FRouteRegistrar(FName InBehaviorKey, std::initializer_list<FName> InOpKeys)
		{
			const FName BehaviorKey = InBehaviorKey;
			TArray<FName> OpKeys;
			OpKeys.Reserve((int32)InOpKeys.size());
			for (const FName& Op : InOpKeys)
			{
				OpKeys.Add(Op);
			}
			GMP::OnGMPModuleLifetime(FSimpleDelegate::CreateLambda([BehaviorKey, OpKeys = MoveTemp(OpKeys)]() {
				if (auto* Mgr = FMessageUtils::GetManager())
				{
					Mgr->GetHub().AddRoute(BehaviorKey, OpKeys);
				}
			}));
		}
	};
}  // namespace GMP::RouteDetail

#define GMP_ROUTE_JOIN_INNER(A, B) A##B
#define GMP_ROUTE_JOIN(A, B) GMP_ROUTE_JOIN_INNER(A, B)
#define GMP_FWD(Key) FName(Key)
#define GMP_ROUTE(BehaviorKey, ...) \
	static ::GMP::RouteDetail::FRouteRegistrar GMP_ROUTE_JOIN(GMPRouteReg_, __LINE__)(FName(BehaviorKey), { __VA_ARGS__ })
