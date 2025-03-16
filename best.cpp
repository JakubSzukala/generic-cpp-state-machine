#include <iostream>
#include <variant>
#include <tuple>

template<template<typename, typename> typename TM, typename ...Ss>
class StateMachine
{
public:
	StateMachine() : states_(Ss(this)...) {}

	template<typename S>
	void setState()
	{
		current_state_ = &std::get<S>(states_);
	}

	template<typename S, typename E>
	void emitEvent(E event)
	{
		setState<typename TM<S, E>::type>();
	}

	void process()
	{
		std::visit([](auto &arg) {arg->process();}, current_state_);
	}

private:
	std::tuple<Ss...> states_;
	std::variant<Ss*...> current_state_ { &std::get<0>(states_) };
};

template<typename SM>
class State
{
public:
	State(SM* sm) : state_machine_(sm) {}
	virtual void process() {};

protected:
	SM* state_machine_;
};


namespace impl
{

/* Forward declarations */
class RunState;
class IdleState;

/* Events */
struct StopEvent {};
struct StartEvent {};

/* Transition map */
template<typename S, typename E>
struct TransitionMap;

template<>
struct TransitionMap<IdleState, StopEvent>
{
	using type = IdleState;
};

template<>
struct TransitionMap<IdleState, StartEvent>
{
	using type = RunState;
};

template<>
struct TransitionMap<RunState, StopEvent>
{
	using type = IdleState;
};

template<>
struct TransitionMap<RunState, StartEvent>
{
	using type = RunState;
};

class IdleState : State<StateMachine<TransitionMap, RunState, IdleState>>
{
public:
	/* Use parent constructor */
	using State<StateMachine<TransitionMap, RunState, IdleState>>::State;
	void process() override;
};

class RunState : State<StateMachine<TransitionMap, RunState, IdleState>>
{
public:
	/* Use parent constructor */
	using State<StateMachine<TransitionMap, RunState, IdleState>>::State;
	void process() override;
};

void IdleState::process()
{
	std::cout << "IDLE" << std::endl;
	state_machine_->emitEvent<IdleState>(StartEvent{});
}

void RunState::process()
{
	std::cout << "RUN" << std::endl;
	state_machine_->emitEvent<RunState>(StopEvent{});
}

} // namespace impl

int main()
{
	StateMachine<impl::TransitionMap, impl::RunState, impl::IdleState> sm;
	/* We start in RunState and RunState emits event inside it's process()
	 * function that will switch to the IdleState */
	sm.process(); // stdout: RUN
	
	/* Idle does not emit any events so we will stay in that state */
	sm.process(); // stdout: IDLE
	sm.process(); // stdout: IDLE
	return 0;
}
