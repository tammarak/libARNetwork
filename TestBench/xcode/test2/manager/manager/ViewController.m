//
//  ViewController.m
//  manager
//
//  Created by Nicolas Payot on 12/11/12.
//  Copyright (c) 2012 Parrot SA. All rights reserved.
//

#import "ViewController.h"
#include <libNetwork/network.h>

//#import "AppDelegate.h"

@interface ViewController ()

@end

@implementation ViewController

@synthesize pAppDelegate;
@synthesize textViewInfo;

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    [textfieldIP setDelegate:self];
    [textfieldChar setDelegate:self];
    [textfieldIntAck setDelegate:self];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self setPAppDelegate:nil];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)clickConnection
{
    /*
    if( ![pAppDelegate connected] )
    {
        [pAppDelegate connection:textfieldIP.text] ;
    }
    else
    {
        
    }*/
    
    [pAppDelegate connection:textfieldIP.text] ;
    
    if( [pAppDelegate connected] )
    {
        //[ buttonConnection setTitle:@"Disconnect" forState:UIControlStateNormal ];
        
        [textfieldChar setEnabled:YES];
        [textfieldIntAck setEnabled:YES];

        [buttonSendChar setEnabled:YES];
        [buttonSendIntAck setEnabled:YES];
        
        //[textfieldIP setEnabled:NO];
    }
   /* else
    {
        //[ buttonConnection setTitle:@"Connect" forState:UIControlStateNormal ];
        
        [textfieldChar setEnabled:NO];
        [textfieldIntAck setEnabled:NO];
        
        [buttonSendChar setEnabled:NO];
        [buttonSendIntAck setEnabled:NO];
    }*/
    
    
}

-(void) touchesBegan :(NSSet *) touches withEvent:(UIEvent *)event
{
    [textfieldIP resignFirstResponder];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    
	[textField resignFirstResponder];
    
	return YES;
}

- (IBAction)clcikSendChar
{
    [pAppDelegate sendChar:textfieldChar.text];
}

- (IBAction)clcikSendIntAck
{
    [pAppDelegate sendIntAck:textfieldIntAck.text];
}


- (IBAction)clcikExit
{
    [pAppDelegate exit];
}


@end