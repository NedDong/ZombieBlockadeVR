// Fill out your copyright notice in the Description page of Project Settings.


#include "Economy.h"

UEconomy* UEconomy::_instance = nullptr;

UEconomy* UEconomy::Instance()
{
	if (!_instance)
	{
		_instance = NewObject<UEconomy>();
		_instance->AddToRoot();
	}
	return _instance;
}

void UEconomy::reset()
{
	if (_instance)
	{
		_instance->RemoveFromRoot();
		_instance = nullptr;
	}
}

int UEconomy::GetMoney()
{
	return this->money;
}

void UEconomy::SetMoney(int value)
{
	this->money = value;
}

void UEconomy::AddMoney(int value)
{
	this->money += value;
}

void UEconomy::RemoveMoney(int value)
{
	this->money -= value;
}

UEconomy::UEconomy() : money(500)
{
}
