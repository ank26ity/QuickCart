import fs from 'fs';
import path from 'path';

export type OrderStatus =
  | 'pending'
  | 'accepted'
  | 'preparing'
  | 'ready'
  | 'assigned'
  | 'picked_up'
  | 'delivered'
  | 'cancelled'
  | 'rejected';

export type OrderActor = 'customer' | 'merchant' | 'courier' | 'admin' | 'system';

interface StateMachineDefinition {
  version: string;
  statuses: OrderStatus[];
  terminalStatuses: OrderStatus[];
  actors: OrderActor[];
  transitions: Array<{
    from: OrderStatus;
    to: OrderStatus;
    allowedActors: OrderActor[];
    description: string;
  }>;
}

export class OrderStateMachineService {
  private static definition: StateMachineDefinition;

  static {
    // Load transitions contract
    const jsonPath = path.resolve(__dirname, '../../../core/order_transitions.json');
    if (fs.existsSync(jsonPath)) {
      this.definition = JSON.parse(fs.readFileSync(jsonPath, 'utf8'));
    } else {
      // Fallback matching exact C++ definition
      this.definition = {
        version: '1.0.0',
        statuses: ['pending', 'accepted', 'preparing', 'ready', 'assigned', 'picked_up', 'delivered', 'cancelled', 'rejected'],
        terminalStatuses: ['delivered', 'cancelled', 'rejected'],
        actors: ['customer', 'merchant', 'courier', 'admin', 'system'],
        transitions: [
          { from: 'pending', to: 'accepted', allowedActors: ['merchant', 'admin'], description: 'Merchant or admin accepts' },
          { from: 'pending', to: 'rejected', allowedActors: ['merchant', 'admin'], description: 'Merchant or admin rejects' },
          { from: 'pending', to: 'cancelled', allowedActors: ['customer', 'admin', 'system'], description: 'Customer or admin cancels' },
          { from: 'accepted', to: 'preparing', allowedActors: ['merchant', 'admin'], description: 'Merchant starts preparing' },
          { from: 'accepted', to: 'cancelled', allowedActors: ['merchant', 'admin'], description: 'Merchant cancels accepted' },
          { from: 'preparing', to: 'ready', allowedActors: ['merchant', 'admin'], description: 'Merchant marks ready' },
          { from: 'preparing', to: 'cancelled', allowedActors: ['admin'], description: 'Admin cancels during preparation' },
          { from: 'ready', to: 'assigned', allowedActors: ['courier', 'admin'], description: 'Courier claims or admin assigns' },
          { from: 'ready', to: 'picked_up', allowedActors: ['courier', 'admin'], description: 'Courier picks up directly' },
          { from: 'ready', to: 'cancelled', allowedActors: ['admin'], description: 'Admin cancels ready' },
          { from: 'assigned', to: 'picked_up', allowedActors: ['courier', 'admin'], description: 'Assigned courier picks up' },
          { from: 'assigned', to: 'ready', allowedActors: ['courier', 'admin'], description: 'Courier unassigns' },
          { from: 'assigned', to: 'cancelled', allowedActors: ['admin'], description: 'Admin cancels assigned' },
          { from: 'picked_up', to: 'delivered', allowedActors: ['courier', 'admin'], description: 'Courier delivers with OTP' },
          { from: 'picked_up', to: 'cancelled', allowedActors: ['admin'], description: 'Admin cancels in-transit' }
        ]
      };
    }
  }

  public static isTerminal(status: OrderStatus): boolean {
    return this.definition.terminalStatuses.includes(status);
  }

  public static canTransition(
    from: OrderStatus,
    to: OrderStatus,
    actor: OrderActor
  ): { allowed: boolean; reason?: string } {
    if (from === to) {
      return { allowed: false, reason: `Order is already in state '${from}'` };
    }

    if (this.isTerminal(from)) {
      return { allowed: false, reason: `Order is in terminal state '${from}' and cannot be modified` };
    }

    const matched = this.definition.transitions.find(t => t.from === from && t.to === to);
    if (!matched) {
      return { allowed: false, reason: `No transition exists from '${from}' to '${to}'` };
    }

    // Role mapping: "shopkeeper" -> "merchant", "delivery" -> "courier"
    const normalizedActor = (actor as string === 'shopkeeper' ? 'merchant' : (actor as string === 'delivery' ? 'courier' : actor)) as OrderActor;

    if (!matched.allowedActors.includes(normalizedActor)) {
      return {
        allowed: false,
        reason: `Actor '${actor}' is not authorized to transition order from '${from}' to '${to}'`
      };
    }

    return { allowed: true };
  }

  public static getDefinition(): StateMachineDefinition {
    return this.definition;
  }
}
